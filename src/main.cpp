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
#include <common/CliApplication.h>
#include <common/Configuration.h>

#include <openssl/ssl.h>

int main(int argc, char *argv[])
{
	CliApplication cliApp( argc, argv );
	if( cliApp.isDiagnosticRun() )
	{
		return cliApp.run();
	}

	Common app( argc, argv, APP, ":/images/id_icon_128x128.png" );
	if( app.isCrashReport() )
		return app.exec();

#ifndef Q_OS_MAC
	if( app.isRunning() )
	{
		app.sendMessage( "" );
		return 0;
	}
#endif
	SSL_library_init();

	MainWindow w;
	Configuration::instance().checkVersion("QESTEIDUTIL");
#ifndef Q_OS_MAC
	QObject::connect( &app, SIGNAL(messageReceived(QString)), &w, SLOT(raiseAndRead()) );
#endif
	w.show();
	if(QWidget *data = w.findChild<QWidget*>("dataWidget"))
		data->setFixedSize(data->geometry().size()); // Hack for avoiding Qt Resize window IB-4242

	return app.exec();
}
