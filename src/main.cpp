/*
 * QEstEidUtil
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

#include <common/Common.h>

#include "MainWindow.h"

#ifdef BREAKPAD
#include <breakpad/QBreakPad.h>
#include <QtCore/QVariant>
#endif

#include <QtGui/QIcon>

#include <openssl/ssl.h>

int main(int argc, char *argv[])
{
	Common app( argc, argv );
	app.setApplicationName( APP );
	app.setApplicationVersion( QString( "%1.%2.%3.%4" )
		.arg( MAJOR_VER ).arg( MINOR_VER ).arg( RELEASE_VER ).arg( BUILD_VER ) );
	app.setOrganizationDomain( DOMAINURL );
	app.setOrganizationName( ORG );
	app.setWindowIcon( QIcon( ":/images/id_icon_128x128.png" ) );
	app.detectPlugins();

#ifdef BREAKPAD
	if( QBreakPad::isCrashReport( argc, argv ) )
	{
		QBreakPadDialog d( app.applicationName() );
		d.setProperty( "User-Agent", QString( "%1/%2 (%3)" )
			.arg( app.applicationName(), app.applicationVersion(), app.applicationOs() ).toUtf8() );
		d.show();
		return app.exec();
	}
	QBreakPad breakpad;
#endif

#ifndef Q_OS_MAC
	if( app.isRunning() )
	{
		app.sendMessage( "" );
		return 0;
	}
#endif

	SSL_library_init();

	MainWindow w;
#ifndef Q_OS_MAC
	QObject::connect( &app, SIGNAL(messageReceived(QString)), &w, SLOT(raiseAndRead()) );
#endif
	w.show();
	return app.exec();
}
