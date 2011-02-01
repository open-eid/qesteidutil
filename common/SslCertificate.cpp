/*
 * QEstEidCommon
 *
 * Copyright (C) 2009,2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009,2010 Raul Metsma <raul@innovaatik.ee>
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

#include <QDateTime>
#include <QLocale>
#include <QMap>
#include <QSslKey>
#include <QStringList>

#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>

static QByteArray ASN_STRING_to_QByteArray( ASN1_OCTET_STRING *str )
{ return QByteArray( (const char *)ASN1_STRING_data(str), ASN1_STRING_length(str) ); }

static QMap<QByteArray, QString> mapFromX509Name( X509_NAME *name )
{
	QMap<QByteArray,QString> info;
	for( int i = 0; i < X509_NAME_entry_count(name); ++i )
	{
		X509_NAME_ENTRY *e = X509_NAME_get_entry( name, i );
		const char *obj = OBJ_nid2sn( OBJ_obj2nid( X509_NAME_ENTRY_get_object( e ) ) );
		unsigned char *data = 0;
		int size = ASN1_STRING_to_UTF8( &data, X509_NAME_ENTRY_get_data( e ) );
		info[obj] = QString::fromUtf8( (char*)data, size );
		OPENSSL_free( data );
	}
	return info;
}



SslCertificate::SslCertificate( const QSslCertificate &cert )
: QSslCertificate( cert ) {}

QByteArray SslCertificate::authorityKeyIdentifier() const
{
	AUTHORITY_KEYID *id = (AUTHORITY_KEYID *)getExtension( NID_authority_key_identifier );
	QByteArray out;
	if( id && id->keyid )
		out = ASN_STRING_to_QByteArray( id->keyid );
	AUTHORITY_KEYID_free( id );
	return out;
}

QHash<SslCertificate::EnhancedKeyUsage,QString> SslCertificate::enhancedKeyUsage() const
{
	QHash<EnhancedKeyUsage,QString> list;
	EXTENDED_KEY_USAGE *usage = (EXTENDED_KEY_USAGE*)getExtension( NID_ext_key_usage );
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
		case NID_email_protect:
			list[EmailProtect] = tr("Protects e-mail messages"); break;
		case NID_OCSP_sign:
			list[OCSPSign] = tr("OCSP signing"); break;
		default: break;
		}
	}
	sk_ASN1_OBJECT_pop_free( usage, ASN1_OBJECT_free );
	return list;
}

QString SslCertificate::formatDate( const QDateTime &date, const QString &format )
{
	int pos = format.indexOf( "MMMM" );
	if( pos == -1 )
		return date.toString( format );
	QString d = date.toString( QString( format ).remove( pos, 4 ) );
	return d.insert( pos, QLocale().monthName( date.date().month() ) );
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
	unsigned char *cert = 0;
	int len = i2d_X509( (X509*)x509, &cert );
	QByteArray der;
	if( len >= 0 )
		der = QByteArray( (char*)cert, len );
	OPENSSL_free( cert );
	return QSslCertificate( der, QSsl::Der );
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

void* SslCertificate::getExtension( int nid ) const
{
	if( !handle() )
		return 0;
	return X509_get_ext_d2i( (X509*)handle(), nid, 0, 0 );
}

QString SslCertificate::issuerInfo( SubjectInfo subject ) const
{ return issuerInfo( subjectInfoToString( subject ) ); }

QString SslCertificate::issuerInfo( const QByteArray &tag ) const
{ return handle() ? mapFromX509Name( X509_get_issuer_name((X509*)handle()) ).value( tag ) : QString(); }

bool SslCertificate::isDigiID() const
{ CertType t = type(); return t == DigiIDType || t==DigiIDTestType; }
bool SslCertificate::isTempel() const { return type() == TempelType; }
bool SslCertificate::isTest() const
{ CertType t = type(); return t == DigiIDTestType || t == EstEidTestType; }

QHash<SslCertificate::KeyUsage,QString> SslCertificate::keyUsage() const
{
	ASN1_BIT_STRING *keyusage = (ASN1_BIT_STRING*)getExtension( NID_key_usage );
	if( !keyusage )
		return QHash<KeyUsage,QString>();

	QHash<KeyUsage,QString> list;
	for( int n = 0; n < 9; ++n )
	{
		if( ASN1_BIT_STRING_get_bit( keyusage, n ) )
		{
			QString usage;
			switch( n )
			{
			case DigitalSignature: usage = tr("Digital signature"); break;
			case NonRepudiation: usage = tr("Non repudiation"); break;
			case KeyEncipherment: usage = tr("Key encipherment"); break;
			case DataEncipherment: usage = tr("Data encipherment"); break;
			case KeyAgreement: usage = tr("Key agreement"); break;
			case KeyCertificateSign: usage = tr("Key certificate sign"); break;
			case CRLSign: usage = tr("CRL sign"); break;
			case EncipherOnly: usage = tr("Encipher only"); break;
			case DecipherOnly: usage = tr("Decipher only"); break;
			default: break;
			}
			if( !usage.isEmpty() )
				list[KeyUsage(n)] = usage;
		}
	}
	ASN1_BIT_STRING_free( keyusage );
	return list;
}

QStringList SslCertificate::policies() const
{
	CERTIFICATEPOLICIES *cp = (CERTIFICATEPOLICIES*)getExtension( NID_certificate_policies );
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

QString SslCertificate::policyInfo( const QString &index ) const
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

QString SslCertificate::signatureAlgorithm() const
{
	if( !handle() )
		return QString();

	char buf[50];
	memset( buf, 0, 50 );
	i2t_ASN1_OBJECT( buf, 50, ((X509*)handle())->cert_info->signature->algorithm );
	return buf;
}

QString SslCertificate::subjectInfo( SubjectInfo subject ) const
{ return subjectInfo( subjectInfoToString( subject ) ); }

QString SslCertificate::subjectInfo( const QByteArray &tag ) const
{ return handle() ? mapFromX509Name( X509_get_subject_name((X509*)handle()) ).value( tag ) : QString(); }

QByteArray SslCertificate::subjectInfoToString( SubjectInfo info ) const
{
	switch( info )
	{
	case QSslCertificate::Organization: return "O";
	case QSslCertificate::CommonName: return "CN";
	case QSslCertificate::LocalityName: return "L";
	case QSslCertificate::OrganizationalUnitName: return "OU";
	case QSslCertificate::CountryName: return "C";
	case QSslCertificate::StateOrProvinceName: return "ST";
	default: return "";
	}
}

QByteArray SslCertificate::subjectKeyIdentifier() const
{
	ASN1_OCTET_STRING *id = (ASN1_OCTET_STRING *)getExtension( NID_subject_key_identifier );
	if( !id )
		return QByteArray();
	QByteArray out = ASN_STRING_to_QByteArray( id );
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
	while( (pos = r.indexIn( format, pos )) != -1 )
	{
		ret.replace( r.cap(0), subjectInfo( r.cap(0).toLatin1() ) );
		pos += r.matchedLength();
	}
	return formatName( ret );
}

SslCertificate::CertType SslCertificate::type() const
{
	if( enhancedKeyUsage().keys().contains( OCSPSign ) )
	{
		return subjectInfo( QSslCertificate::CommonName ).indexOf( QRegExp( "TEST\\-SK.*OCSP.*") ) != -1 ?
			OCSPTestType : OCSPType;
	}

	QStringList p = policies();
	if( p.indexOf( QRegExp( "^1\\.3\\.6\\.1\\.4\\.1\\.10015\\.1\\.1.*" ) ) != -1 )
		return EstEidType;
	if( p.indexOf( QRegExp( "^1\\.3\\.6\\.1\\.4\\.1\\.10015\\.1\\.2.*" ) ) != -1 )
		return DigiIDType;
	if( p.indexOf( QRegExp( "^1\\.3\\.6\\.1\\.4\\.1\\.10015\\.3\\.1.*" ) ) != -1 )
		return EstEidTestType;
	if( p.indexOf( QRegExp( "^1\\.3\\.6\\.1\\.4\\.1\\.10015\\.3\\.2.*" ) ) != -1 )
		return DigiIDTestType;
	if( p.indexOf( QRegExp( "^1\\.3\\.6\\.1\\.4\\.1\\.10015\\.7.*" ) ) != -1 )
		return TempelType;
	return UnknownType;
}



class PKCS12CertificatePrivate
{
public:
	PKCS12CertificatePrivate(): error(PKCS12Certificate::NullError) {}
	void init( const QByteArray &data, const QByteArray &pin );
	void setLastError();

	QSslCertificate cert;
	QSslKey key;
	PKCS12Certificate::ErrorType error;
	QString errorString;
};

void PKCS12CertificatePrivate::init( const QByteArray &data, const QByteArray &pin )
{
	BIO *bio = BIO_new_mem_buf( const_cast<char*>(data.constData()), data.size() );
	if( !bio )
		return setLastError();

	PKCS12 *p12 = d2i_PKCS12_bio( bio, 0 );
	BIO_free( bio );
	if( !p12 )
		return setLastError();

	X509 *c = 0;
	EVP_PKEY *k = 0;
	int ret = PKCS12_parse( p12, pin.constData(), &k, &c, 0 );
	PKCS12_free( p12 );
	if( !ret )
		return setLastError();
	// Hack: clear PKCS12_parse error ERROR: 185073780 - error:0B080074:x509 certificate routines:X509_check_private_key:key values mismatch
	ERR_get_error();

	cert = SslCertificate::fromX509( Qt::HANDLE(c) );
	key = SslCertificate::keyFromEVP( Qt::HANDLE(k) );

	X509_free( c );
	EVP_PKEY_free( k );
}

void PKCS12CertificatePrivate::setLastError()
{
	unsigned long err = ERR_get_error();
	if( err == 0 )
	{
		   error = PKCS12Certificate::NullError;
		   errorString.clear();
	}
	else if( ERR_GET_LIB(err) == ERR_LIB_PKCS12 )
	{
		switch( ERR_GET_REASON(err) )
		{
		case PKCS12_R_MAC_VERIFY_FAILURE: error = PKCS12Certificate::InvalidPasswordError; break;
		default: error = PKCS12Certificate::UnknownError; break;
		}
	}
	else
	{
		error = PKCS12Certificate::UnknownError;
		errorString = ERR_error_string( err, 0 );
	}
}



PKCS12Certificate::PKCS12Certificate( QIODevice *device, const QByteArray &pin )
:	d(new PKCS12CertificatePrivate)
{ if( device ) d->init( device->readAll(), pin ); }

PKCS12Certificate::PKCS12Certificate( const QByteArray &data, const QByteArray &pin )
:	d(new PKCS12CertificatePrivate)
{ d->init( data, pin ); }

PKCS12Certificate::~PKCS12Certificate() { delete d; }
QSslCertificate PKCS12Certificate::certificate() const { return d->cert; }
PKCS12Certificate::ErrorType PKCS12Certificate::error() const { return d->error; }
QString PKCS12Certificate::errorString() const { return d->errorString; }
bool PKCS12Certificate::isNull() const { return d->cert.isNull() && d->key.isNull(); }
QSslKey PKCS12Certificate::key() const { return d->key; }
