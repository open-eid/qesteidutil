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

#include <QtCore/QtGlobal>

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
typedef QApplication BaseApplication;
#else
#include "qtsingleapplication/src/QtSingleApplication"
typedef QtSingleApplication BaseApplication;
#endif

#include <QtCore/QEvent>

class QLabel;
class QSslCertificate;
class QUrl;

class Common: public BaseApplication
{
	Q_OBJECT
public:
	Common( int &argc, char **argv );
	virtual ~Common();

	void detectPlugins();

	static void addRecent( const QString &file );
	static QString applicationOs();
	static bool cardsOrder( const QString &s1, const QString &s2 );
	static QUrl helpUrl();
	static QStringList packages( const QStringList &names, bool withName = true );
	static void setAccessibleName( QLabel *widget );
	static void showHelp( const QString &msg, int code = -1 );

public Q_SLOTS:
	void browse( const QUrl &url );
	void mailTo( const QUrl &url );
	void showPlugins();

protected:
	virtual bool event( QEvent *e );
	void validate();

private:
	static quint8 cardsOrderScore( QChar c );

#if defined(Q_OS_MAC)
	void initMacEvents();
	void deinitMacEvents();
	bool macEvents;
#endif
};

class REOpenEvent: public QEvent
{
public:
	enum { Type = QEvent::User + 1 };
	REOpenEvent(): QEvent( QEvent::Type(Type) ) {}
};
