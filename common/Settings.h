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

#pragma once

#include <QtCore/QSettings>

#include <QtCore/QLocale>

class Settings : public QSettings
{
	Q_OBJECT

public:
#ifdef Q_OS_MAC
	Settings( const QString & = "" ): QSettings() {}
#else
	Settings( QObject *parent = 0 )
	: QSettings( "Estonian ID Card", QString(), parent ) {}
	Settings( const QString &application, QObject *parent = 0 )
	: QSettings( "Estonian ID Card", application, parent ) {}
#endif

	static QString language()
	{
		QString deflang;
		switch( QLocale().language() )
		{
		case QLocale::English: deflang = "en"; break;
		case QLocale::Russian: deflang = "ru"; break;
		case QLocale::Estonian:
		default: deflang = "et"; break;
		}
		return Settings().value( "Main/Language", deflang ).toString();
	}
};
