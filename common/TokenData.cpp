/*
 * QDigiDocCommon
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

#include "SslCertificate.h"

#include <QDateTime>
#include <QStringList>
#include <QTextStream>

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

void TokenData::clear() { d = new TokenDataPrivate; }

TokenData::TokenFlags TokenData::flags() const { return d->flags; }
void TokenData::setFlag( TokenFlags flag, bool enabled )
{
	if( enabled ) d->flags |= flag;
	else d->flags &= ~flag;
}
void TokenData::setFlags( TokenFlags flags ) { d->flags = flags; }

QString TokenData::toHtml() const
{
	QString content;
	QTextStream s( &content );
	SslCertificate c( d->cert );

	s << "<table width=\"100%\"><tr><td>";
	if( c.isTempel() )
	{
		s << tr("Company") << ": <font color=\"black\">"
			<< c.toString( "CN" ) << "</font><br />";
		s << tr("Register code") << ": <font color=\"black\">"
			<< c.subjectInfo( "serialNumber" ) << "</font><br />";
	}
	else
	{
		s << tr("Name") << ": <font color=\"black\">"
			<< c.toString( "GN SN" ) << "</font><br />";
		s << tr("Personal code") << ": <font color=\"black\">"
			<< c.subjectInfo( "serialNumber" ) << "</font><br />";
	}
	s << tr("Card in reader") << ": <font color=\"black\">" << d->card << "</font><br />";

	bool willExpire = c.expiryDate().toLocalTime() <= QDateTime::currentDateTime().addDays( 105 );
	s << (c.keyUsage().keys().contains( SslCertificate::NonRepudiation ) ? tr("Sign certificate is") : tr("Auth certificate is")  ) << " ";
	if( c.isValid() )
	{
		s << "<font color=\"green\">" << tr("valid") << "</font>";
		if( willExpire )
			s << "<br /><font color=\"red\">" << tr("Your certificates will expire soon") << "</font>";
	}
	else
		s << "<font color=\"red\">" << tr("expired") << "</font>";
	if( d->flags & TokenData::PinLocked )
		s << "<br /><font color=\"red\">" << tr("PIN is locked") << "</font>";

	s << "</td><td align=\"center\" width=\"75\">";
	if( !c.isValid() || willExpire || d->flags & TokenData::PinLocked )
	{
		s << "<a href=\"openUtility\"><img src=\":/images/warning.png\"><br />"
			"<font color=\"red\">" << tr("Open utility") << "</font></a>";
	}
	else if( c.isTempel() )
		s << "<img src=\":/images/ico_stamp_blue_75.png\">";
	else
		s << "<img src=\":/images/ico_person_blue_75.png\">";
	s << "</td></tr></table>";

	return content;
}

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
