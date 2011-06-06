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
		DeviceError,
		GeneralError,
		UnknownError,
	};

	explicit QPKCS11( QObject *parent = 0 );
	~QPKCS11();

	QStringList cards();
	QByteArray encrypt( const QByteArray &data ) const;
	QByteArray decrypt( const QByteArray &data ) const;
	Qt::HANDLE key();
	bool loadDriver( const QString &driver );
	PinStatus login( const TokenData &t );
	bool logout() const;
	TokenData selectSlot( const QString &card, SslCertificate::KeyUsage usage );
	QByteArray sign( int type, const QByteArray &digest ) const;
	void unloadDriver();
	bool verify( const QByteArray &data, const QByteArray &signature ) const;

	static QString errorString( PinStatus error );
private:
	QPKCS11Private *d;
};
