/*
 * QEstEidCommon
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

#include <QObject>

#include <QFileDialog>

class QSslCertificate;
class QStringList;
class QUrl;
class TokenData;

class Common: public QObject
{
	Q_OBJECT
public:
	enum CertType
	{
		AuthCert,
		SignCert,
	};
	Common( QObject *parent = 0 );

	static bool canWrite( const QString &filename );
	static QString fileSize( quint64 bytes );
	static QString normalized( const QString &str );
	static QStringList normalized( const QStringList &list );
	static void showHelp( const QString &msg );
	static bool startDetached( const QString &program );
	static bool startDetached( const QString &program, const QStringList &arguments );
	static QString tokenInfo( CertType type, const TokenData &data );

public Q_SLOTS:
	void browse( const QUrl &url );
	void mailTo( const QUrl &url );
};

#ifdef Q_OS_MAC
#include <QEvent>

class REOpenEvent: public QEvent
{
public:
	enum { Type = QEvent::User + 1 };
	REOpenEvent(): QEvent( QEvent::Type(Type) ) {}
};
#endif
