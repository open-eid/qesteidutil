/*
 * QEstEidCommon
 *
 * Copyright (C) 2009-2011 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009-2011 Raul Metsma <raul@innovaatik.ee>
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
#include "SOAPDocument.h"
#include "SslCertificate.h"

#include <QApplication>
#include <QProgressBar>
#include <QProgressDialog>

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
				return;
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
,	error( SSLConnect::NoError )
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
	{
		error = SSLConnect::PKCS11Error;
		return false;
	}

	if( !selectSlot() )
	{
		error = SSLConnect::PKCS11Error;
		errorString = SSLConnect::tr("no token available");
		return false;
	}

	// Find token cert
	PKCS11_CERT *certs;
	unsigned int ncerts;
	if( PKCS11_enumerate_certs( pslot->token, &certs, &ncerts ) || !ncerts )
	{
		error = SSLConnect::PKCS11Error;
		errorString = SSLConnect::tr("no certificate available");
		return false;
	}
	QSslCertificate cert = SslCertificate::fromX509( Qt::HANDLE((&certs[0])->x509) );
	if( !cert.isValid() )
	{
		error = SSLConnect::PKCS11Error;
		errorString = SSLConnect::tr("Certificate is not valid");
		return false;
	}

	// Login token
	if( pslot->token->loginRequired )
	{
		unsigned long err = CKR_OK;
		if( !pslot->token->secureLogin )
		{
			PinDialog p( PinDialog::Pin1Type, cert, flags, qApp->activeWindow() );
			if( !p.exec() )
			{
				error = SSLConnect::PinCanceledError;
				errorString = SSLConnect::tr("PIN canceled");
				return false;
			}
			if( PKCS11_login( pslot, 0, p.text().toUtf8() ) < 0 )
				err = ERR_get_error();
		}
		else
		{
			PinDialog p( PinDialog::Pin1PinpadType, cert, flags, qApp->activeWindow() );
			PINPADThread t( pslot );
			QObject::connect( &t, SIGNAL(started()), &p, SLOT(startTimer()) );
			p.open();
			err = t.waitForDone();
		}
		switch( ERR_GET_REASON(err) )
		{
		case CKR_OK: break;
		case CKR_CANCEL:
		case CKR_FUNCTION_CANCELED:
			error = SSLConnect::PinCanceledError;
			errorString = SSLConnect::tr("PIN canceled");
			return false;
		case CKR_PIN_INCORRECT:
			selectSlot();
			error = SSLConnect::PinInvalidError;
			errorString = SSLConnect::tr("Invalid PIN");
			return false;
		case CKR_PIN_LOCKED:
			error = SSLConnect::PinLockedError;
			errorString = SSLConnect::tr("Pin locked");
			return false;
		default:
			error = SSLConnect::PinInvalidError;
			errorString = SSLConnect::tr("Invalid PIN");
			return false;
		}
	}

	// Find token key
	PKCS11_KEY *authkey = PKCS11_find_key( &certs[0] );
	if ( !authkey )
	{
		error = SSLConnect::PKCS11Error;
		errorString = SSLConnect::tr("no key matching certificate available");
		return false;
	}
	EVP_PKEY *pkey = PKCS11_get_private_key( authkey );

	ssl = SSL_new( SSL_CTX_new( SSLv23_client_method() ) );
	if( !SSL_use_certificate( ssl, (&certs[0])->x509 ) )
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::getError();
		return false;
	}
	if( !SSL_use_PrivateKey( ssl, pkey ) )
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::getError();
		return false;
	}
	if( !SSL_check_private_key( ssl ) )
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::getError();
		return false;
	}
	if( !SSL_set_mode( ssl, SSL_MODE_AUTO_RETRY ) )
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::getError();
		return false;
	}

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
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::tr( "Failed to connect to host. Are you connected to the internet?" );
		return false;
	}

	SSL_set_bio( ssl, sock, sock );
	
	if ( !SSL_connect( ssl ) )
	{
		error = SSLConnect::SSLError;
		errorString = SSLConnect::getError();
		return false;
	}

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
		SOAPDocument s( "GetAccessToken", "urn:GetAccessToken" );
		s.writeParameter( "Language", Settings::language().toUpper() );
		s.writeParameter( "RequestTime", "" );
		s.writeParameter( "SoftwareName", "DigiDoc3" );
		s.writeParameter( "SoftwareVersion", qApp->applicationVersion() );
		s.finalize();
		header = QString(
			"POST /GetAccessTokenWS/ HTTP/1.1\r\n"
			"Host: %1\r\n"
			"Content-Type: text/xml\r\n"
			"Content-Length: %2\r\n"
			"SOAPAction: \"\"\r\n"
			"Connection: close\r\n\r\n"
			"%3" )
			.arg( SK ).arg( s.document().size() ).arg( QString::fromUtf8( s.document() ) );
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
	if( !SSL_write( d->ssl, data.constData(), data.length() ) )
	{
		d->error = SSLConnect::SSLError;
		d->errorString = SSLConnect::getError();
		return QByteArray();
	}
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
QString SSLConnect::getError() { return ERR_reason_error_string( ERR_get_error() ); }
bool SSLConnect::setCard( const QString &card ) { d->card = card; return d->selectSlot(); }
void SSLConnect::setPKCS11( const QString &pkcs11, bool unload )
{
	if( d->p11loaded && d->unload )
		PKCS11_CTX_unload( d->p11 );
	d->unload = unload;
	d->p11loaded = PKCS11_CTX_load( d->p11, pkcs11.toUtf8() ) == 0;
	if( !d->p11loaded )
	{
		d->error = PKCS11Error;
		d->errorString = tr("failed to load pkcs11 module '%1'").arg( pkcs11 );
	}
}
