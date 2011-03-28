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

#include <QThread>

#include <libp11.h>

#include <openssl/ssl.h>

// PKCS#11
#define CKR_OK					(0)
#define CKR_CANCEL				(1)
#define CKR_FUNCTION_CANCELED	(0x50)
#define CKR_PIN_INCORRECT		(0xa0)
#define CKR_PIN_LOCKED			(0xa4)

class SSLConnectPrivate
{
public:
	SSLConnectPrivate();
	~SSLConnectPrivate();

	bool connectToHost( const QByteArray &url );
	bool selectSlot();
	void setError( SSLConnect::ErrorType type, const QString &msg = QString() );

	bool	unload;
	PKCS11_CTX *p11;
	bool	p11loaded;
	SSL		*ssl;

	QString card;
	TokenData::TokenFlags flags;
	PKCS11_SLOT *pslot;

	unsigned int nslots;
	PKCS11_SLOT *pslots;

	SSLConnect::ErrorType error;
	QString errorString;
};

class PINPADThread: public QThread
{
	Q_OBJECT
public:
	explicit PINPADThread( PKCS11_SLOT *slot, QObject *parent = 0 )
	: QThread(parent), result(CKR_OK), m_slot(slot) {}

	unsigned long waitForDone();

private:
	void run();

	unsigned long result;
	PKCS11_SLOT *m_slot;
};

class SSLReadThread: public QThread
{
	Q_OBJECT
public:
	explicit SSLReadThread( SSL *ssl, QObject *parent = 0 ): QThread( parent ), m_ssl(ssl) {}

	QByteArray waitForDone();

private:
	void run();

	QByteArray result;
	SSL *m_ssl;
};
