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

#include <QObject>

#include "TokenData.h"

#define EESTI "sisene.www.eesti.ee"
#define OPENXADES "www.openxades.org"
#define SK "id.sk.ee"

class SSLConnectPrivate;
class SSLConnect: public QObject
{
	Q_OBJECT

public:
	enum RequestType {
		EmailInfo,
		ActivateEmails,
		MobileInfo,
		PictureInfo,
		AccessCert
	};
	enum ErrorType {
		PinCanceledError = 1,
		PinInvalidError = 2,
		PinLockedError = 3,
		NoError = -1,
		PKCS11Error = 4,
		SSLError = 5,
	};

	SSLConnect( QObject *parent = 0 );
	~SSLConnect();

	ErrorType error() const;
	QString errorString() const;
	TokenData::TokenFlags flags() const;
	QByteArray getUrl( RequestType type, const QString &value = "" );
	QString pin() const;
	static QString getError();
	bool setCard( const QString &card );
	void setPin( const QString &pin );
	void setPKCS11( const QString &pkcs11, bool unload = true );

private:
	SSLConnectPrivate	*d;
};
