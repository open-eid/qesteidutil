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

#include "sslConnect_p.h"

#include "PinDialog.h"
#include "Settings.h"
#include "SslCertificate.h"

#include <stdexcept>
#include <sstream>
#include <QApplication>
#include <QProgressBar>
#include <QProgressDialog>

class sslError: public std::runtime_error
{
public:
	sslError() : std::runtime_error("openssl error")
	{
		unsigned long err = ERR_get_error();
		std::ostringstream buf;
		buf << ERR_func_error_string( err ) << " (" << ERR_reason_error_string( err ) << ")";
		desc = buf.str();
	}
    ~sslError() throw () {}

	virtual const char *what() const throw() { return desc.c_str(); }
	void static check( bool result ) { if( !result ) throw sslError(); }
	void static error( const QString &msg ) { throw std::runtime_error( msg.toStdString() ); }

	std::string desc;
};



void PINPADThread::run()
{
	if( PKCS11_login( m_slot, 0, 0 ) < 0 )
		result = ERR_get_error();
}

unsigned long PINPADThread::waitForDone()
{
	QEventLoop e;
	connect( this, SIGNAL(finished()), &e, SLOT(quit()) );
	start();
	e.exec();
	return result;
}



void SSLReadThread::run()
{
	int bytesRead = 0;
	char readBuffer[4096];
	QByteArray buffer;
	do
	{
		bytesRead = SSL_read( m_ssl, &readBuffer, 4096 );

		if( bytesRead <= 0 )
		{
			switch( SSL_get_error( m_ssl, bytesRead ) )
			{
			case SSL_ERROR_NONE:
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_ZERO_RETURN: // Disconnect
				break;
			default:
				sslError::check( true );
				break;
			}
		}

		if( bytesRead > 0 )
			buffer += QByteArray( (const char*)&readBuffer, bytesRead );
	} while( bytesRead > 0 );

	int pos = buffer.indexOf( "\r\n\r\n" );
	result = pos ? buffer.mid( pos + 4 ) : buffer;
}

QByteArray SSLReadThread::waitForDone()
{
	QEventLoop e;
	connect( this, SIGNAL(finished()), &e, SLOT(quit()) );
	start();
	e.exec();
	return result;
}



SSLConnectPrivate::SSLConnectPrivate()
:	unload( true )
,	p11( PKCS11_CTX_new() )
,	p11loaded( false )
,	ssl( 0 )
,	flags( 0 )
,	nslots( 0 )
,	error( SSLConnect::UnknownError )
{}

SSLConnectPrivate::~SSLConnectPrivate()
{
	if( ssl )
		SSL_shutdown( ssl );
	SSL_free( ssl );
	if( nslots )
		PKCS11_release_all_slots( p11, pslots, nslots );
	if( p11loaded && unload )
		PKCS11_CTX_unload( p11 );
	PKCS11_CTX_free( p11 );
}

bool SSLConnectPrivate::connectToHost( SSLConnect::RequestType type )
{
	if( !p11loaded )
		sslError::error( errorString.toUtf8() );

	if( !selectSlot() )
		sslError::error( SSLConnect::tr("no token available").toUtf8() );

	// Find token cert
	PKCS11_CERT *certs;
	unsigned int ncerts;
	if( PKCS11_enumerate_certs( pslot->token, &certs, &ncerts ) || !ncerts )
		sslError::error( SSLConnect::tr("no certificate available").toUtf8() );
	QSslCertificate cert = SslCertificate::fromX509( Qt::HANDLE((&certs[0])->x509) );
	if( !cert.isValid() )
		sslError::error( SSLConnect::tr("Certificate is not valid").toUtf8() );

	// Login token
	if( pslot->token->loginRequired )
	{
		unsigned long err = CKR_OK;
		if( !pslot->token->secureLogin )
		{
			PinDialog p( PinDialog::Pin1Type, cert, flags, qApp->activeWindow() );
			if( !p.exec() )
				throw std::runtime_error( "" );
			if( PKCS11_login( pslot, 0, p.text().toUtf8() ) < 0 )
				err = ERR_get_error();
		}
		else
		{
			PinDialog p( PinDialog::Pin1PinpadType, cert, flags, qApp->activeWindow() );
			p.open();
			err = PINPADThread( pslot ).waitForDone();
		}
		switch( ERR_GET_REASON(err) )
		{
		case CKR_OK: break;
		case CKR_CANCEL:
		case CKR_FUNCTION_CANCELED:
			throw std::runtime_error( "" ); break;
		case CKR_PIN_INCORRECT:
			selectSlot();
			throw std::runtime_error( "PIN1Invalid" ); break;
		case CKR_PIN_LOCKED:
			throw std::runtime_error( "PINLocked" ); break;
		default:
			throw std::runtime_error( "PIN1Invalid" ); break;
		}
	}

	// Find token key
	PKCS11_KEY *authkey = PKCS11_find_key( &certs[0] );
	if ( !authkey )
		sslError::error( SSLConnect::tr("no key matching certificate available").toUtf8() );
	EVP_PKEY *pkey = PKCS11_get_private_key( authkey );

	ssl = SSL_new( SSL_CTX_new( SSLv23_client_method() ) );
	sslError::check( SSL_use_certificate( ssl, (&certs[0])->x509 ) );
	sslError::check( SSL_use_PrivateKey( ssl, pkey ) );
	sslError::check( SSL_check_private_key( ssl ) );
	sslError::check( SSL_set_mode( ssl, SSL_MODE_AUTO_RETRY ) );

	const char *url = 0;
	switch( type )
	{
	case SSLConnect::AccessCert:
	case SSLConnect::MobileInfo: url = SK; break;
	default: url = EESTI; break;
	}
	BIO *sock = BIO_new_connect( const_cast<char*>(url) );

	BIO_set_conn_port( sock, "https" );
	if( BIO_do_connect( sock ) <= 0 )
		sslError::error( SSLConnect::tr( "Failed to connect to host. Are you connected to the internet?" ).toUtf8() );

	SSL_set_bio( ssl, sock, sock );
	sslError::check( SSL_connect( ssl ) );

	return true;
}

bool SSLConnectPrivate::selectSlot()
{
	if( !p11loaded )
		return false;

	if( nslots )
	{
		PKCS11_release_all_slots( p11, pslots, nslots );
		nslots = 0;
	}
	if( PKCS11_enumerate_slots( p11, &pslots, &nslots ) || !nslots )
		return false;

	pslot = 0;
	for( unsigned int i = 0; i < nslots; ++i )
	{
		if( (&pslots[i])->token &&
			card.contains( (&pslots[i])->token->serialnr ) )
		{
			pslot = &pslots[i];
			break;
		}
	}

	if( !pslot || !pslot->token )
		return false;

#ifdef LIBP11_TOKEN_FLAGS
	if( pslot->token->soPinCountLow || pslot->token->userPinCountLow )
		flags |= TokenData::PinCountLow;
	if( pslot->token->soPinFinalTry || pslot->token->userPinFinalTry )
		flags |= TokenData::PinFinalTry;
	if( pslot->token->soPinLocked || pslot->token->userPinLocked )
		flags |= TokenData::PinLocked;
#endif
	return true;
}



SSLConnect::SSLConnect( QObject *parent )
:	QObject( parent )
,	d( new SSLConnectPrivate() )
{
	setPKCS11( PKCS11_MODULE );
}

SSLConnect::~SSLConnect() { delete d; }

QByteArray SSLConnect::getUrl( RequestType type, const QString &value )
{
	if( !d->connectToHost( type ) )
		return QByteArray();

	QString header;
	QString label;
	switch( type )
	{
	case AccessCert:
	{
		label = tr("Loading server access certificate. Please wait.");
		QString request = QString(
			"<SOAP-ENV:Envelope"
			"	xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\""
			"	xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\""
			"	xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
			"	xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			"<SOAP-ENV:Body>"
			"<m:GetAccessToken"
			"	xmlns:m=\"urn:GetAccessToken\""
			"	SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
			"<Language xsi:type=\"xsd:string\">%1</Language>"
			"<RequestTime xsi:type=\"xsd:string\" />"
			"<SoftwareName xsi:type=\"xsd:string\">DigiDoc3</SoftwareName>"
			"<SoftwareVersion xsi:type=\"xsd:string\" />"
			"</m:GetAccessToken>"
			"</SOAP-ENV:Body>"
			"</SOAP-ENV:Envelope>" )
			.arg( Settings::language().toUpper() );
		header = QString(
			"POST /GetAccessTokenWS/ HTTP/1.1\r\n"
			"Host: %1\r\n"
			"Content-Type: text/xml\r\n"
			"Content-Length: %2\r\n"
			"SOAPAction: \"\"\r\n"
			"Connection: close\r\n\r\n"
			"%3" )
			.arg( SK ).arg( request.size() ).arg( request );
		break;
	}
	case EmailInfo:
		label = tr("Loading Email info");
		header = "GET /idportaal/postisysteem.naita_suunamised HTTP/1.0\r\n\r\n";
		break;
	case ActivateEmails:
		label = tr("Loading Email info");
		header = QString( "GET /idportaal/postisysteem.lisa_suunamine?%1 HTTP/1.0\r\n\r\n" ).arg( value );
		break;
	case MobileInfo:
		label = tr("Loading Mobile info");
		header = value;
		break;
	case PictureInfo:
		label = tr("Downloading picture");
		header = "GET /idportaal/portaal.idpilt HTTP/1.0\r\n\r\n";
		break;
	default: return QByteArray();
	}

	QByteArray data = header.toUtf8();
	sslError::check( SSL_write( d->ssl, data.constData(), data.length() ) );
	QProgressDialog p( label, QString(), 0, 0, qApp->activeWindow() );
	p.setWindowFlags( (p.windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint );
	if( QProgressBar *bar = p.findChild<QProgressBar*>() )
		bar->setTextVisible( false );
	p.open();
	return SSLReadThread( d->ssl ).waitForDone();
}

SSLConnect::ErrorType SSLConnect::error() const { return d->error; }
QString SSLConnect::errorString() const { return d->errorString; }
TokenData::TokenFlags SSLConnect::flags() const { return d->flags; }
QByteArray SSLConnect::result() const { return d->result; }
void SSLConnect::setCard( const QString &card ) { d->card = card; d->selectSlot(); }
void SSLConnect::setPKCS11( const QString &pkcs11, bool unload )
{
	if( d->p11loaded && d->unload )
		PKCS11_CTX_unload( d->p11 );
	d->unload = unload;
	d->p11loaded = PKCS11_CTX_load( d->p11, pkcs11.toUtf8() ) == 0;
	if( !d->p11loaded )
	{
		d->error = UnknownError;
		d->errorString = tr("failed to load pkcs11 module '%1'").arg( pkcs11 );
	}
}

void SSLConnect::waitForFinished( RequestType type, const QString &value )
{
	try { d->result = getUrl( type, value ); }
	catch( const std::runtime_error &e )
	{
		if( qstrcmp( e.what(), "" ) == 0 )
		{
			d->error = SSLConnect::PinCanceledError;
			d->errorString = tr("PIN canceled");
		}
		else if( qstrcmp( e.what(), "PIN1Invalid" ) == 0 )
		{
			d->error = SSLConnect::PinInvalidError;
			d->errorString = tr("Invalid PIN");
		}
		else if( qstrcmp( e.what(), "PINLocked" ) == 0 )
		{
			d->error = SSLConnect::PinLocked;
			d->errorString = tr("Pin locked");
		}
		else
		{
			d->error = SSLConnect::UnknownError;
			d->errorString = QString::fromUtf8( e.what() );
		}
		return;
	}
	d->error = SSLConnect::UnknownError;
	d->errorString.clear();
}
