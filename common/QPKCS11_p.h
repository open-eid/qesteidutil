/*
 * QDigiDocCommon
 *
 * Copyright (C) 2010-2011 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010-2011 Raul Metsma <raul@innovaatik.ee>
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

#include "QPKCS11.h"

#include "pkcs11.h"

#include <QEventLoop>
#include <QLibrary>
#include <QThread>

#include <openssl/rsa.h>

class QPKCS11Private
{
public:
	QPKCS11Private();

	bool attribute( CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_TYPE type, void *value, unsigned long &size ) const;
	QByteArray attribute( CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_TYPE type ) const;
	BIGNUM* attribute_bn( CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_TYPE type ) const;
	QSslCertificate readCert( CK_SLOT_ID slot );
	bool findObject( CK_OBJECT_CLASS cls, CK_OBJECT_HANDLE *ret ) const;
	void freeSlotIds();
	bool getSlotsIds();

	static int rsa_sign( int type, const unsigned char *m, unsigned int m_len,
		unsigned char *sigret, unsigned int *siglen, const RSA *rsa );

	QLibrary		lib;
	CK_FUNCTION_LIST *f;
	CK_SLOT_ID		*pslot, *pslots;
	CK_SESSION_HANDLE session;
	unsigned long	nslots;

	RSA_METHOD method;
};

class QPKCS11Thread: public QThread
{
	Q_OBJECT
public:
	explicit QPKCS11Thread( QPKCS11Private *p, QObject *parent = 0 )
	: QThread(parent), d(p), result(CKR_OK) {}

	unsigned long waitForDone()
	{
		QEventLoop e;
		connect( this, SIGNAL(finished()), &e, SLOT(quit()) );
		start();
		e.exec();
		return result;
	}

private:
	void run() { result = d->f->C_Login( d->session, CKU_USER, 0, 0 ); }

	QPKCS11Private *d;
	CK_RV result;
};
