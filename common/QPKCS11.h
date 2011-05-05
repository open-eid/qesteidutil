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

#include <QObject>

#include "SslCertificate.h"

class TokenData;
class QPKCS11Private;

class QPKCS11: public QObject
{
    Q_OBJECT
public:
	enum PinStatus
	{
		PinOK,
		PinCanceled,
		PinIncorrect,
		PinLocked,
		PinUnknown,
	};

	explicit QPKCS11( QObject *parent = 0 );
	~QPKCS11();

	QStringList cards();
	bool encrypt( const QByteArray &data, unsigned char *encrypted, unsigned long *len );
	bool decrypt( const QByteArray &data, unsigned char *decrypted, unsigned long *len );
	Qt::HANDLE key();
	bool loadDriver( const QString &driver );
	PinStatus login( const TokenData &t );
	bool logout();
	TokenData selectSlot( const QString &card, SslCertificate::KeyUsage usage );
	QByteArray sign( const QByteArray &data );
	void unloadDriver();
	bool verify( const QByteArray &data, unsigned char *signature, unsigned long len );

private:
	QPKCS11Private *d;
};
