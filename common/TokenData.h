/*
 * QDigiDocClient
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

#pragma once

#include <QMetaType>
#include <QSharedDataPointer>

class QSslCertificate;
class TokenDataPrivate;
class TokenData
{
public:
	enum TokenFlag
	{
		PinCountLow = (1<<1),
		PinFinalTry = (1<<2),
		PinLocked = (1<<3),
	};
	Q_DECLARE_FLAGS(TokenFlags, TokenFlag)

	TokenData();
	TokenData( const TokenData &other );
	~TokenData();

	QString card() const;
	void setCard( const QString &card );

	QStringList cards() const;
	void setCards( const QStringList &cards );

	QSslCertificate cert() const;
	void setCert( const QSslCertificate &cert );

	void clear();

	TokenFlags flags() const;
	void setFlag( TokenFlags flag, bool enabled );
	void setFlags( TokenFlags flags );

	TokenData operator =( const TokenData &other );
	bool operator !=( const TokenData &other ) const;
	bool operator ==( const TokenData &other ) const;

private:
	QSharedDataPointer<TokenDataPrivate> d;
};

Q_DECLARE_METATYPE(TokenData)
