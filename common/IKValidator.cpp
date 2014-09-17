/*
 * QEstEidCommon
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

#include "IKValidator.h"

#include <QtCore/QDate>
#include <QtCore/QStringList>

IKValidator::IKValidator( QObject *parent )
:	QValidator( parent )
{}

QDate IKValidator::birthDate( const QString &ik )
{
	if( ik.size() != 11 ) return QDate();

	quint16 year = 0;
	switch( ik.left( 1 ).toUInt() )
	{
	case 1: case 2: year = 1800; break;
	case 3: case 4: year = 1900; break;
	case 5: case 6: year = 2000; break;
	case 7: case 8: year = 2100; break;
	default: return QDate();
	}

	QDate date(
		ik.mid( 1, 2 ).toUInt() + year,
		ik.mid( 3, 2 ).toUInt(),
		ik.mid( 5, 2 ).toUInt() );
	return date.isValid() ? date : QDate();
}

bool IKValidator::isValid( const QString &ik )
{
	if( ik.size() != 11 )
		return false;

	// Mobile-ID test IK-s
	static const QStringList list = QStringList()
		<< "14212128020"
		<< "14212128021"
		<< "14212128022"
		<< "14212128023"
		<< "14212128024"
		<< "14212128025"
		<< "14212128026"
		<< "14212128027"
		<< "38002240211"
		<< "14212128029";
	if( list.contains( ik ) )
		return true;

	// Validate date
	if( birthDate( ik ).isNull() )
		return false;

	// Validate checksum
	int sum1 = 0, sum2 = 0, pos1 = 1, pos2 = 3;
	for( int i = 0; i < 10; ++i )
	{
		sum1 += ik.mid( i, 1 ).toUInt() * pos1;
		sum2 += ik.mid( i, 1 ).toUInt() * pos2;
		pos1 = pos1 == 9 ? 1 : pos1 + 1;
		pos2 = pos2 == 9 ? 1 : pos2 + 1;
	}

	int result;
	if( (result = sum1 % 11) >= 10 &&
		(result = sum2 % 11) >= 10 )
		result = 0;

	return ik.right( 1 ).toInt() == result;
}

QValidator::State IKValidator::validate( QString &input, int & ) const
{
	input = input.trimmed();
	if( input.size() > 11 || !QRegExp( "\\d{0,11}" ).exactMatch( input ) )
		return Invalid;
	else if( input.size() == 11 )
		return isValid( input ) ? Acceptable : Invalid;
	else
		return Intermediate;
}



NumberValidator::NumberValidator( QObject *parent )
	: QValidator( parent )
{}

NumberValidator::State NumberValidator::validate( QString &input, int & ) const
{
	QString out;
	QRegExp rx( "(\\d+)" );
	int p = 0;
	while( (p = rx.indexIn( input, p )) != -1 )
	{
		out += input.mid( p, rx.matchedLength() );
		p += rx.matchedLength();
	}
	input = out;
	return Acceptable;
}
