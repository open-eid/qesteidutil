/*
 * QEstEidCommon
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "SslCertificate.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtNetwork/QSslKey>

#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>

uint qHash( const SslCertificate &cert ) { return qHash( cert.digest() ); }

SslCertificate::SslCertificate()
: QSslCertificate() {}

SslCertificate::SslCertificate( const QByteArray &data, QSsl::EncodingFormat format )
: QSslCertificate( data, format ) {}

SslCertificate::SslCertificate( const QSslCertificate &cert )
: QSslCertificate( cert ) {}

#if QT_VERSION >= 0x050000
QString SslCertificate::issuerInfo( const QByteArray &tag ) const
{ return QSslCertificate::issuerInfo( tag ).value(0); }

QString SslCertificate::issuerInfo( QSslCertificate::SubjectInfo subject ) const
 { return QSslCertificate::issuerInfo( subject ).value(0); }

QString SslCertificate::subjectInfo( const QByteArray &tag ) const
 { return QSslCertificate::subjectInfo( tag ).value(0); }

QString SslCertificate::subjectInfo( QSslCertificate::SubjectInfo subject ) const
{ return QSslCertificate::subjectInfo( subject ).value(0); }
#endif

QByteArray SslCertificate::authorityKeyIdentifier() const
{
	AUTHORITY_KEYID *id = (AUTHORITY_KEYID*)extension( NID_authority_key_identifier );
	QByteArray out;
	if( id && id->keyid )
		out = QByteArray( (const char*)id->keyid->data, id->keyid->length );
	AUTHORITY_KEYID_free( id );
	return out;
}

bool SslCertificate::canUpdate() const
{
	return type() & EstEidType && publicKey().length() <= 1024 &&
		std::max<int>( 0, QDateTime::currentDateTime().daysTo( expiryDate().toLocalTime() ) ) <= 105;
}

QHash<SslCertificate::EnhancedKeyUsage,QString> SslCertificate::enhancedKeyUsage() const
{
	QHash<EnhancedKeyUsage,QString> list;
	EXTENDED_KEY_USAGE *usage = (EXTENDED_KEY_USAGE*)extension( NID_ext_key_usage );
	if( !usage )
	{
		list[All] = tr("All application policies");
		return list;
	}

	for( int i = 0; i < sk_ASN1_OBJECT_num( usage ); ++i )
	{
		ASN1_OBJECT *obj = sk_ASN1_OBJECT_value( usage, i );
		switch( OBJ_obj2nid( obj ) )
		{
		case NID_client_auth:
			list[ClientAuth] = tr("Proves your identity to a remote computer"); break;
		case NID_server_auth:
			list[ServerAuth] = tr("Ensures the identity of a remote computer"); break;
		case NID_email_protect:
			list[EmailProtect] = tr("Protects e-mail messages"); break;
		case NID_OCSP_sign:
			list[OCSPSign] = tr("OCSP signing"); break;
		case NID_time_stamp:
			list[TimeStamping] = tr("Time Stamping"); break;
		default: break;
		}
	}
	sk_ASN1_OBJECT_pop_free( usage, ASN1_OBJECT_free );
	return list;
}

QString SslCertificate::friendlyName() const
{
	QString cn = subjectInfo( QSslCertificate::CommonName );
	QString o = subjectInfo( QSslCertificate::Organization );
	if( o == "ESTEID" ) return QString( "%1,%2" ).arg( cn, tr("ID-CARD") );
	if( o == "ESTEID (DIGI-ID)" ) return QString( "%1,%2" ).arg( cn, tr("DIGI-ID") );
	if( o == "ESTEID (MOBIIL-ID)" ) return QString( "%1,%2" ).arg( cn, tr("MOBIIL-ID") );
	return cn;
}

QString SslCertificate::formatName( const QString &name )
{
	QString ret = name.toLower();
	bool firstChar = true;
	for( QString::iterator i = ret.begin(); i != ret.end(); ++i )
	{
		if( !firstChar && !i->isLetter() )
			firstChar = true;

		if( firstChar && i->isLetter() )
		{
			*i = i->toUpper();
			firstChar = false;
		}
	}
	return ret;
}

QSslCertificate SslCertificate::fromX509( Qt::HANDLE x509 )
{
	QByteArray der( i2d_X509( (X509*)x509, 0 ), 0 );
	if( !der.isEmpty() )
	{
		unsigned char *p = (unsigned char*)der.data();
		i2d_X509( (X509*)x509, &p );
	}
	return QSslCertificate( der, QSsl::Der );
}

bool SslCertificate::isCA() const
{
	BASIC_CONSTRAINTS *cons = (BASIC_CONSTRAINTS*)extension( NID_basic_constraints );
	if( !cons )
		return false;
	bool result = cons->ca > 0;
	BASIC_CONSTRAINTS_free(cons);
	return result;
}

QSslKey SslCertificate::keyFromEVP( Qt::HANDLE evp )
{
	EVP_PKEY *key = (EVP_PKEY*)evp;
	unsigned char *data = 0;
	int len = 0;
	QSsl::KeyAlgorithm alg;
	QSsl::KeyType type;

	switch( EVP_PKEY_type( key->type ) )
	{
	case EVP_PKEY_RSA:
	{
		RSA *rsa = EVP_PKEY_get1_RSA( key );
		alg = QSsl::Rsa;
		type = rsa->d ? QSsl::PrivateKey : QSsl::PublicKey;
		len = rsa->d ? i2d_RSAPrivateKey( rsa, &data ) : i2d_RSAPublicKey( rsa, &data );
		RSA_free( rsa );
		break;
	}
	case EVP_PKEY_DSA:
	{
		DSA *dsa = EVP_PKEY_get1_DSA( key );
		alg = QSsl::Dsa;
		type = dsa->priv_key ? QSsl::PrivateKey : QSsl::PublicKey;
		len = dsa->priv_key ? i2d_DSAPrivateKey( dsa, &data ) : i2d_DSAPublicKey( dsa, &data );
		DSA_free( dsa );
		break;
	}
	default: break;
	}

	QSslKey k;
	if( len > 0 )
		k = QSslKey( QByteArray( (char*)data, len ), alg, QSsl::Der, type );
	OPENSSL_free( data );

	return k;
}

QString SslCertificate::keyName() const
{
	X509 *c = (X509*)handle();
	EVP_PKEY *key = X509_PUBKEY_get( c->cert_info->key );
	QString name = tr("Unknown");
	switch( EVP_PKEY_type( key->type ) )
	{
	case EVP_PKEY_DSA:
		name = QString("DSA (%1)").arg( publicKey().length() );
		break;
	case EVP_PKEY_RSA:
		name = QString("RSA (%1)").arg( publicKey().length() );
		break;
#ifndef OPENSSL_NO_ECDSA
	case EVP_PKEY_EC:
	{
		EC_KEY *ec = EVP_PKEY_get1_EC_KEY( key );

		int nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec));
		ASN1_OBJECT *obj = OBJ_nid2obj(nid);
		char buff[1024];
		if(OBJ_obj2txt(buff, 1024, obj, 0) > 0)
			name = buff;

		EC_KEY_free( ec );
		break;
	}
#endif
	default: break;
	}
	EVP_PKEY_free( key );
	return name;
}

Qt::HANDLE SslCertificate::extension( int nid ) const
{ return !isNull() ? Qt::HANDLE(X509_get_ext_d2i( (X509*)handle(), nid, 0, 0 )) : 0; }

QHash<SslCertificate::KeyUsage,QString> SslCertificate::keyUsage() const
{
	ASN1_BIT_STRING *keyusage = (ASN1_BIT_STRING*)extension( NID_key_usage );
	if( !keyusage )
		return QHash<KeyUsage,QString>();

	QHash<KeyUsage,QString> list;
	for( int n = 0; n < 9; ++n )
	{
		if( !ASN1_BIT_STRING_get_bit( keyusage, n ) )
			continue;
		switch( n )
		{
		case DigitalSignature: list[KeyUsage(n)] = tr("Digital signature"); break;
		case NonRepudiation: list[KeyUsage(n)] = tr("Non repudiation"); break;
		case KeyEncipherment: list[KeyUsage(n)] = tr("Key encipherment"); break;
		case DataEncipherment: list[KeyUsage(n)] = tr("Data encipherment"); break;
		case KeyAgreement: list[KeyUsage(n)] = tr("Key agreement"); break;
		case KeyCertificateSign: list[KeyUsage(n)] = tr("Key certificate sign"); break;
		case CRLSign: list[KeyUsage(n)] = tr("CRL sign"); break;
		case EncipherOnly: list[KeyUsage(n)] = tr("Encipher only"); break;
		case DecipherOnly: list[KeyUsage(n)] = tr("Decipher only"); break;
		default: break;
		}
	}
	ASN1_BIT_STRING_free( keyusage );
	return list;
}

QStringList SslCertificate::policies() const
{
	CERTIFICATEPOLICIES *cp = (CERTIFICATEPOLICIES*)extension( NID_certificate_policies );
	if( !cp )
		return QStringList();

	QStringList list;
	for( int i = 0; i < sk_POLICYINFO_num( cp ); ++i )
	{
		POLICYINFO *pi = sk_POLICYINFO_value( cp, i );
		char buf[50];
		memset( buf, 0, 50 );
		int len = OBJ_obj2txt( buf, 50, pi->policyid, 1 );
		if( len != NID_undef )
			list << buf;
	}
	sk_POLICYINFO_pop_free( cp, POLICYINFO_free );
	return list;
}

QString SslCertificate::policyInfo( const QString & ) const
{
#if 0
	for( int j = 0; j < sk_POLICYQUALINFO_num( pi->qualifiers ); ++j )
	{
		POLICYQUALINFO *pqi = sk_POLICYQUALINFO_value( pi->qualifiers, j );

		memset( buf, 0, 50 );
		int len = OBJ_obj2txt( buf, 50, pqi->pqualid, 1 );
		qDebug() << buf;
	}
#endif
	return QString();
}

QString SslCertificate::publicKeyHex() const
{
	X509 *x = static_cast<X509*>(handle());
	if(!x)
		return QString();
	EVP_PKEY *key = X509_PUBKEY_get( x->cert_info->key );
	QString hex;
	switch( EVP_PKEY_type( key->type ) )
	{
	case EVP_PKEY_EC:
	{
		EC_KEY *ec = EVP_PKEY_get1_EC_KEY(key);
		QByteArray key(i2d_EC_PUBKEY(ec, 0), 0);
		unsigned char *p = (unsigned char*)key.data();
		i2d_EC_PUBKEY(ec, &p);
		EC_KEY_free(ec);
		hex = toHex(key);
		break;
	}
	default:
		hex = toHex( publicKey().toDer() );
		break;
	}
	EVP_PKEY_free( key );
	return hex;
}

QByteArray SslCertificate::serialNumber( bool hex ) const
{
	if( isNull() )
		return QByteArray();

	QByteArray serial;
	if( BIGNUM *bn = ASN1_INTEGER_to_BN( X509_get_serialNumber( (X509*)handle() ), 0 ) )
	{
		if( char *str = hex ? BN_bn2hex( bn ) : BN_bn2dec( bn ) )
		{
			serial = str;
			OPENSSL_free( str );
		}
		BN_free( bn );
	}
	return serial;
}

bool SslCertificate::showCN() const
{ return subjectInfo( "GN" ).isEmpty() && subjectInfo( "SN" ).isEmpty(); }

QString SslCertificate::signatureAlgorithm() const
{
	if( isNull() )
		return QString();

	char buf[50];
	memset( buf, 0, 50 );
	i2t_ASN1_OBJECT( buf, 50, ((X509*)handle())->cert_info->signature->algorithm );
	return buf;
}

QByteArray SslCertificate::subjectKeyIdentifier() const
{
	ASN1_OCTET_STRING *id = (ASN1_OCTET_STRING*)extension( NID_subject_key_identifier );
	if( !id )
		return QByteArray();
	QByteArray out = QByteArray( (const char*)id->data, id->length );
	ASN1_OCTET_STRING_free( id );
	return out;
}

QByteArray SslCertificate::toHex( const QByteArray &in, QChar separator )
{
	QByteArray ret = in.toHex().toUpper();
	for( int i = 2; i < ret.size(); i += 3 )
		ret.insert( i, separator );
	return ret;
}

QString SslCertificate::toString( const QString &format ) const
{
	QRegExp r( "[a-zA-Z]+" );
	QString ret = format;
	int pos = 0;
	while( (pos = r.indexIn( ret, pos )) != -1 )
	{
		QString si = subjectInfo( r.cap(0).toLatin1() );
		ret.replace( pos, r.cap(0).size(), si );
		pos += si.size();
	}
	return showCN() ? ret : formatName( ret );
}

SslCertificate::CertType SslCertificate::type() const
{
	for(const QString &p: policies())
	{
		if( enhancedKeyUsage().keys().contains( OCSPSign ) )
		{
			return
				p.startsWith( "1.3.6.1.4.1.10015.3." ) ||
				subjectInfo( QSslCertificate::CommonName ).indexOf( "TEST" ) != -1 ?
				OCSPTestType : OCSPType;
		}

		if( p.startsWith( "1.3.6.1.4.1.10015.1.1." ) )
			return EstEidType;
		if( p.startsWith( "1.3.6.1.4.1.10015.1.2." ) )
			return DigiIDType;
		if( p.startsWith( "1.3.6.1.4.1.10015.1.3." ) ||
			p.startsWith( "1.3.6.1.4.1.10015.11.1." ) )
			return MobileIDType;

		if( p.startsWith( "1.3.6.1.4.1.10015.3.1." ) )
			return EstEidTestType;
		if( p.startsWith( "1.3.6.1.4.1.10015.3.2." ) )
			return DigiIDTestType;
		if( p.startsWith( "1.3.6.1.4.1.10015.3.3." ) ||
			p.startsWith( "1.3.6.1.4.1.10015.3.11." ) )
			return MobileIDTestType;
		if( p.startsWith( "1.3.6.1.4.1.10015.3.7." ) ||
			(p.startsWith( "1.3.6.1.4.1.10015.7.1." ) &&
			 issuerInfo( QSslCertificate::CommonName ).indexOf( "TEST" ) != -1) )
			return TempelTestType;

		if( p.startsWith( "1.3.6.1.4.1.10015.7.1." ) ||
			p.startsWith( "1.3.6.1.4.1.10015.2.1." ) )
			return TempelType;
	}
	return UnknownType;
}



class PKCS12CertificatePrivate: public QSharedData
{
public:
	PKCS12CertificatePrivate(): error(PKCS12Certificate::NullError) {}
	void init( const QByteArray &data, const QString &pin );
	void setLastError();

	QList<QSslCertificate> caCerts;
	QSslCertificate cert;
	QSslKey key;
	PKCS12Certificate::ErrorType error;
	QString errorString;
};

void PKCS12CertificatePrivate::init( const QByteArray &data, const QString &pin )
{
	const unsigned char *p = (const unsigned char*)data.constData();
	PKCS12 *p12 = d2i_PKCS12( 0, &p, data.size() );
	if( !p12 )
		return setLastError();

	STACK_OF(X509) *ca = 0;
	X509 *c = 0;
	EVP_PKEY *k = 0;
	QByteArray _pin = pin.toUtf8();
	int ret = PKCS12_parse( p12, _pin.constData(), &k, &c, &ca );
	PKCS12_free( p12 );
	if( !ret )
		return setLastError();
	// Hack: clear PKCS12_parse error ERROR: 185073780 - error:0B080074:x509 certificate routines:X509_check_private_key:key values mismatch
	ERR_get_error();

	cert = SslCertificate::fromX509( Qt::HANDLE(c) );
	key = SslCertificate::keyFromEVP( Qt::HANDLE(k) );
	for(int i = 0; i < sk_X509_num(ca); ++i)
		caCerts << SslCertificate::fromX509(Qt::HANDLE(sk_X509_value(ca, i)));

	X509_free( c );
	EVP_PKEY_free( k );
	sk_X509_free( ca );
}

void PKCS12CertificatePrivate::setLastError()
{
	error = PKCS12Certificate::NullError;
	errorString.clear();
	while( ERR_peek_error() > ERR_LIB_NONE )
	{
		unsigned long err = ERR_get_error();
		if( ERR_GET_LIB(err) == ERR_LIB_PKCS12 &&
			ERR_GET_REASON(err) == PKCS12_R_MAC_VERIFY_FAILURE )
		{
			error = PKCS12Certificate::InvalidPasswordError;
			return;
		}
		error = PKCS12Certificate::UnknownError;
		errorString += ERR_error_string( err, 0 );
	}
}



PKCS12Certificate::PKCS12Certificate( QIODevice *device, const QString &pin )
:	d(new PKCS12CertificatePrivate)
{ if( device ) d->init( device->readAll(), pin ); }

PKCS12Certificate::PKCS12Certificate( const QByteArray &data, const QString &pin )
:	d(new PKCS12CertificatePrivate)
{ d->init( data, pin ); }

PKCS12Certificate::PKCS12Certificate( const PKCS12Certificate &other ): d(other.d) {}

PKCS12Certificate::~PKCS12Certificate() {}
QList<QSslCertificate> PKCS12Certificate::caCertificates() const { return d->caCerts; }
QSslCertificate PKCS12Certificate::certificate() const { return d->cert; }
PKCS12Certificate::ErrorType PKCS12Certificate::error() const { return d->error; }
QString PKCS12Certificate::errorString() const { return d->errorString; }

PKCS12Certificate PKCS12Certificate::fromPath( const QString &path, const QString &pin )
{
	PKCS12Certificate p12( 0, QString() );
	QFile f( path );
	if( !f.exists() )
		p12.d->error = PKCS12Certificate::FileNotExist;
	else if( !f.open( QFile::ReadOnly ) )
		p12.d->error = PKCS12Certificate::FailedToRead;
	else
		p12.d->init( f.readAll(), pin );
	return p12;
}

bool PKCS12Certificate::isNull() const { return d->cert.isNull() && d->key.isNull(); }
QSslKey PKCS12Certificate::key() const { return d->key; }
