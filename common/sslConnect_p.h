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

#pragma once

#include "sslConnect.h"

#include <QNetworkRequest>
#include <QThread>

#include <openssl/err.h>
#include <openssl/ssl.h>

#define EESTI "sisene.www.eesti.ee"
#define OPENXADES "www.openxades.org"
#define SK "id.sk.ee"

class HTTPRequest: public QNetworkRequest
{
public:
	HTTPRequest(): QNetworkRequest() {}
	HTTPRequest( const QByteArray &method, const QByteArray &ver, const QUrl &url )
	: QNetworkRequest( url ), m_method( method ), m_ver( ver ) {}

	void setContent( const QByteArray &data ) { m_data = data; }
	QByteArray request() const;

private:
	QByteArray m_data, m_method, m_ver;
};

class SSLConnectPrivate
{
public:
	SSLConnectPrivate(): ctx(0), ssl(0) {}

	void setError( const QString &msg = QString() )
	{ errorString = msg.isEmpty() ? ERR_reason_error_string( ERR_get_error() ) : msg; }

	SSL_CTX *ctx;
	SSL		*ssl;
	QString errorString;
};

class SSLReadThread: public QThread
{
	Q_OBJECT
public:
	explicit SSLReadThread( SSLConnectPrivate *ssl, QObject *parent = 0 ): QThread( parent ), d(ssl) {}

	QByteArray waitForDone();

private:
	void run();

	QByteArray result;
	SSLConnectPrivate *d;
};
