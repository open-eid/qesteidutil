/*
 * QEstEidUtil
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

#include <qtsingleapplication.h>

#include "mainwindow.h"
#include "version.h"

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
	AllowSetForegroundWindow( ASFW_ANY );
#endif
	QtSingleApplication app(argc, argv);
	app.setApplicationName( APP );
	app.setApplicationVersion( VER_STR( FILE_VER_DOT ) );
	app.setOrganizationDomain( DOMAINURL );
	app.setOrganizationName( ORG );
	app.setWindowIcon( QIcon( ":/html/images/id_icon_big.png" ) );

	if( app.isRunning() )
	{
		app.sendMessage( "" );
		return 0;
	}

	MainWindow w;
	QObject::connect( &app, SIGNAL(messageReceived(QString)), &w, SLOT(raiseAndRead()) );
    w.show();
    return app.exec();
}
