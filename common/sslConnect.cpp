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

#include "Settings.h"
#include "SOAPDocument.h"

#include <common/Common.h>

#include <QDebug>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSslCertificate>

QByteArray HTTPRequest::request() const
{
	QByteArray r;
	r += m_method + " " + url().toEncoded( QUrl::RemoveScheme|QUrl::RemoveAuthority ) + " HTTP/" + m_ver + "\r\n";
	r += "Host: " + url().host() + "\r\n";
	r += "User-Agent: " + QString( "%1/%2 (%3)\r\n" )
		.arg( qApp->applicationName() ).arg( qApp->applicationVersion() ).arg( Common::applicationOs() ).toUtf8();
	foreach( const QByteArray &header, rawHeaderList() )
		r += header + ": " + rawHeader( header ) + "\r\n";

	if( !m_data.isEmpty() )
	{
		r += "Content-Length: " + QByteArray::number( m_data.size() ) + "\r\n\r\n";
		r += m_data;
	}
	else
		r += "\r\n";

	return r;
}



void SSLReadThread::run()
{
	int bytesRead = 0;
	char readBuffer[4096];
	QByteArray buffer;
	do
	{
		bytesRead = SSL_read( d->ssl, &readBuffer, 4096 );

		if( bytesRead <= 0 )
		{
			switch( SSL_get_error( d->ssl, bytesRead ) )
			{
			case SSL_ERROR_NONE:
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_ZERO_RETURN: // Disconnect
				break;
			default:
				d->setError();
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



SSLConnect::SSLConnect( QObject *parent )
:	QObject( parent )
,	d( new SSLConnectPrivate() )
{
	d->ctx = SSL_CTX_new( TLSv1_client_method() );
	if( d->ctx )
		d->ssl = SSL_new( d->ctx );
	if( d->ssl )
		SSL_set_mode( d->ssl, SSL_MODE_AUTO_RETRY );
	else
		d->setError();
}

SSLConnect::~SSLConnect()
{
	if( d->ssl )
		SSL_shutdown( d->ssl );
	SSL_free( d->ssl );
	delete d;
}

QByteArray SSLConnect::getUrl( RequestType type, const QString &value )
{
	if( !d->ssl )
		return QByteArray();

	if( !SSL_check_private_key( d->ssl ) )
	{
		d->setError();
		return QByteArray();
	}

	QString label;
	HTTPRequest req;
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
		req = HTTPRequest( "POST", "1.1", QString("https://%1/GetAccessTokenWS/").arg( SK ) );
		req.setRawHeader( "Content-Type", "text/xml" );
		req.setRawHeader( "SOAPAction", QByteArray() );
		req.setRawHeader( "Connection", "close" );
		req.setContent( s.document() );
		break;
	}
	case EmailInfo:
		label = tr("Loading Email info");
		req = HTTPRequest( "GET", "1.0",
			QString("https://%1/idportaal/postisysteem.naita_suunamised").arg( EESTI ) );
		break;
	case ActivateEmails:
		label = tr("Loading Email info");
		req = HTTPRequest( "GET", "1.0",
			QString("https://%1/idportaal/postisysteem.lisa_suunamine?%2").arg( EESTI ).arg( value ) );
		break;
	case MobileInfo:
	{
		label = tr("Loading Mobile info");
		SOAPDocument s( "GetMIDTokens", "urn:GetMIDTokens" );
		s.finalize();
		req = HTTPRequest( "POST", "1.1", QString("https://%1/MIDInfoWS/").arg( SK ) );
		req.setRawHeader( "Content-Type", "text/xml" );
		req.setRawHeader( "SOAPAction", QByteArray() );
		req.setRawHeader( "Connection", "close" );
		req.setContent( s.document() );
		break;
	}
	case PictureInfo:
		label = tr("Downloading picture");
		req = HTTPRequest( "GET", "1.0", QString("https://%1/idportaal/portaal.idpilt").arg( EESTI ) );
		break;
	default: return QByteArray();
	}

	QByteArray url = req.url().host().toUtf8();
	BIO *sock = BIO_new_connect( (char*)url.constData() );
	BIO_set_conn_port( sock, "https" );
	if( BIO_do_connect( sock ) <= 0 )
	{
		d->setError( tr( "Failed to connect to host. Are you connected to the internet?" ) );
		return QByteArray();
	}

	SSL_set_bio( d->ssl, sock, sock );
	if( !SSL_connect( d->ssl ) )
	{
		d->setError();
		return QByteArray();
	}

	QByteArray header = req.request();
	if( !SSL_write( d->ssl, header.constData(), header.size() ) )
	{
		d->setError();
		return QByteArray();
	}

	QProgressDialog p( label, QString(), 0, 0, qApp->activeWindow() );
	p.setWindowFlags( (p.windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint );
	if( QProgressBar *bar = p.findChild<QProgressBar*>() )
		bar->setTextVisible( false );
	p.open();

	return SSLReadThread( d ).waitForDone();
}

QString SSLConnect::errorString() const { return d->errorString; }

void SSLConnect::setToken( const QSslCertificate &cert, Qt::HANDLE key )
{
	if( !d->ssl )
		return;
	if( !SSL_use_certificate( d->ssl, X509_dup( (X509*)cert.handle() ) ) ||
		!SSL_use_PrivateKey( d->ssl, (EVP_PKEY*)key ) )
		d->setError();
}
