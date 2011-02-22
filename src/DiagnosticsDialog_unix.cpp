/*
 * QEstEidUtil
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

#include "DiagnosticsDialog.h"

#include <QFile>
#include <QTextStream>

#if defined(Q_OS_LINUX)
#include <QProcess>
#elif defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#include <QProcess>
#endif

DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QString info;
	QTextStream s( &info );

	s << "<b>" << tr("Locale:") << "</b> ";
	QString locale = QLocale::c().name();
	s << (locale == "C" ? "en_us" : locale) << "<br /><br />";

#if defined(Q_OS_LINUX)
	QString package = getPackageVersion( QStringList() << "estonianidcard", false );
	QString utility = getPackageVersion( QStringList() << "qesteidutil", false );
	if ( !package.isEmpty() )
		s << "<b>" << tr("ID-card package version:") << "</b> " << package << "<br />";
	if ( !utility.isEmpty() )
		s << "<b>" << tr("ID-card utility version:") << "</b> " << utility << "<br />";
#else
	s << "<b>" << tr("ID-card utility version:") << "</b> " << QCoreApplication::applicationVersion() << "<br />";
#endif

	s << "<b>" << tr("OS:") << "</b> ";
#if defined(Q_OS_LINUX)
	QProcess p;
	p.start( "lsb_release", QStringList() << "-s" << "-d" );
	p.waitForReadyRead();
	s << p.readAll();
#elif defined(Q_OS_MAC)
	SInt32 major, minor, bugfix;
	
	if( Gestalt(gestaltSystemVersionMajor, &major) == noErr &&
			Gestalt(gestaltSystemVersionMinor, &minor) == noErr &&
			Gestalt(gestaltSystemVersionBugFix, &bugfix) == noErr )
		s << "Mac OS " << major << "." << minor << "." << bugfix;
	else
		s << "Mac OS 10.3";
#endif

	s << " (" << QSysInfo::WordSize << ")<br />";
#if defined(Q_OS_LINUX)
	s << "<b>" << tr("CPU:") << "</b> " << getProcessor() << "<br /><br />";
#endif

	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";

	s << "<b>" << tr("Libraries") << ":</b><br />";
#if defined(Q_OS_MAC)
	QProcess p;
	p.start( "/Library/OpenSC/bin/opensc-tool", QStringList() << "-i" );
	p.waitForReadyRead();
	s << p.readAll() << "<br />";
#elif defined(Q_OS_LINUX)
	s << getPackageVersion( QStringList() << "openssl" << "libpcsclite1" << "opensc" );
#endif
	s << "QT (" << qVersion() << ")<br />" << "<br />";

	s << "<b>" << tr("PCSC service status: ") << "</b>" << " " << (isPCSCRunning() ? tr("Running") : tr("Not running")) << "<br /><br />";

	s << "<b>" << tr("Card readers") << ":</b><br />" << getReaderInfo() << "<br />";

	QString browsers = getBrowsers();
	if ( !browsers.isEmpty() )
		s << "<b>" << tr("Browsers:") << "</b><br />" << browsers << "<br /><br />";

	diagnosticsText->setHtml( info );
}

QString DiagnosticsDialog::getBrowsers() const
{
#if defined(Q_OS_LINUX)
	return getPackageVersion( QStringList() << "chromium-browser" << "firefox" << "MozillaFirefox");
#elif defined(Q_OS_MAC)
	return getPackageVersion( QStringList() << "Google Chrome" << "Firefox" << "Safari" );
#endif
}

QString DiagnosticsDialog::getPackageVersion( const QStringList &list, bool returnPackageName ) const
{
	QString ret;
#if defined(Q_OS_LINUX)
	QStringList params;
	QProcess p;
	p.start( "which", QStringList() << "dpkg-query" );
	p.waitForReadyRead();
	QByteArray cmd = p.readAll();
	if ( cmd.isEmpty() )
	{
		p.start( "which", QStringList() << "rpm" );
		p.waitForReadyRead();
		cmd = p.readAll();
		if ( cmd.isEmpty() )
			return ret;
		cmd = "rpm";
		params << "-q" << "--qf" << "%{VERSION}";
	} else {
		cmd = "dpkg-query";
		params << "-W" << "-f=${Version}";
	}
	p.close();

	Q_FOREACH( QString package, list )
	{
		p.start( cmd, QStringList() << params << package );
		p.waitForFinished();
		if ( p.exitCode() )
			continue;
		QByteArray result = p.readAll();
		if ( !result.isEmpty() )
		{
			if ( returnPackageName )
				ret += package + " ";
			ret += result + "<BR />";
		}
		p.close();
	}
#elif defined(Q_OS_MAC)
	QProcess p;
	Q_FOREACH( QString package, list )
	{
		QString app = "/Applications/" + package + ".app/Contents/Info";
		if ( !QFile::exists( app + ".plist") )
			continue;
		p.start( "defaults", QStringList() << "read" << app << "CFBundleShortVersionString" );
		p.waitForFinished();
		if ( p.exitCode() )
			continue;
		QByteArray result = p.readAll();
		if ( !result.isEmpty() )
		{
			if ( returnPackageName )
				ret += package + " ";
			ret += result + "<BR />";
		}
		p.close();
	}
#endif

	return ret;
}

QString DiagnosticsDialog::getProcessor() const
{
	QProcess p;
	p.start( "sh -c \"cat /proc/cpuinfo | grep -m 1 model\\ name\"" );
	p.waitForReadyRead();
	return p.readAll();
}

bool DiagnosticsDialog::isPCSCRunning() const
{
	QProcess p;
#if defined(Q_OS_LINUX)
	p.start( "pidof", QStringList() << "pcscd" );
#elif defined(Q_OS_MAC)
	p.start( "sh -c \"ps ax | grep -v grep | grep pcscd\"" );
#else
	return true;
#endif
	p.waitForFinished();
	return !p.readAll().trimmed().isEmpty();
}
