/*
 * QDigiDocClient
 *
 * Copyright (C) 2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010 Raul Metsma <raul@innovaatik.ee>
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

#include "TokenData.h"

#include <QSslCertificate>
#include <QStringList>

class TokenDataPrivate: public QSharedData
{
public:
	TokenDataPrivate(): flags(0) {}

	QString card;
	QStringList cards;
	QSslCertificate cert;
	TokenData::TokenFlags flags;
};



TokenData::TokenData(): d( new TokenDataPrivate ) {}

TokenData::TokenData( const TokenData &other ): d( other.d ) {}

TokenData::~TokenData() {}

QString TokenData::card() const { return d->card; }

void TokenData::setCard( const QString &card ) { d->card = card; }

QStringList TokenData::cards() const { return d->cards; }

void TokenData::setCards( const QStringList &cards ) { d->cards = cards; }

QSslCertificate TokenData::cert() const { return d->cert; }

void TokenData::setCert( const QSslCertificate &cert ) { d->cert = cert; }

TokenData::TokenFlags TokenData::flags() const { return d->flags; }

void TokenData::setFlag( TokenFlags flag, bool enabled)
{
	if( enabled ) d->flags |= flag;
	else d->flags &= ~flag;
}

void TokenData::setFlags( TokenFlags flags ) { d->flags = flags; }

TokenData TokenData::operator =( const TokenData &other ) { d = other.d; return *this; }

bool TokenData::operator !=( const TokenData &other ) const { return !(operator==(other)); }

bool TokenData::operator ==( const TokenData &other ) const
{
	return d == other.d ||
		( d->card == other.d->card &&
		  d->cards == other.d->cards &&
		  d->cert == other.d->cert &&
		  d->flags == other.d->flags );
}
