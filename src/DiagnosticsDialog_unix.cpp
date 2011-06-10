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

#include "DiagnosticsDialog.h"

#include <common/Common.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTextStream>
#include <QProcess>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

static QByteArray runProcess( const QString &program, const QStringList &arguments = QStringList() )
{
	QProcess p;
	if( arguments.isEmpty() )
		p.start( program );
	else
		p.start( program, arguments );
	p.waitForFinished();
	return p.readAll();
}

DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QString info;
	QTextStream s( &info );

	s << "<b>" << tr("Locale:") << "</b> ";
	QLocale::Language language = QLocale::system().language();
	QString locale = QString( "%1 - ").arg( language == QLocale::C ? "English/United States" : QLocale::languageToString( language ) );
#ifdef Q_OS_MAC
	locale.append( runProcess( "sh -c \"echo $LC_CTYPE\"" ) );
#else
	locale.append( runProcess( "sh -c \"echo $LANG\"" ) );
#endif
	s << locale << "<br /><br />";

	QStringList package = getPackageVersion( QStringList() << "estonianidcard", false );
	QStringList utility = getPackageVersion( QStringList() << "qesteidutil", false );
	if ( !package.isEmpty() )
		s << "<b>" << tr("ID-card package version:") << "</b> " << package.first() << "<br />";
	if ( !utility.isEmpty() )
		s << "<b>" << tr("ID-card utility version:") << "</b> " << utility.first() << "<br />";

	s << "<b>" << tr("OS:") << "</b> " << Common::applicationOs() << "<br />";
	s << "<b>" << tr("CPU:") << "</b> " << getProcessor() << "<br /><br />";
	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";
	s << "<b>" << tr("Libraries") << ":</b><br />";
	s << getPackageVersion( QStringList() << "libdigidoc" << "libdigidocpp" ).join( "<br />" ) << "<br />";
#ifdef Q_OS_MAC
	s << runProcess(  "/Library/OpenSC/bin/opensc-tool", QStringList() << "-i" ) << "<br />";
#else
	s << getPackageVersion( QStringList() << "openssl" << "libpcsclite1" << "pcsc-lite" << "opensc" ).join( "<br />" ) << "<br />";
#endif
	s << "QT (" << qVersion() << ")<br />" << "<br />";

	s << "<b>" << tr("PCSC service status: ") << "</b>" << " " << (isPCSCRunning() ? tr("Running") : tr("Not running")) << "<br /><br />";

	s << "<b>" << tr("Card readers") << ":</b><br />" << getReaderInfo() << "<br />";

#ifdef Q_OS_LINUX
	QStringList browsers = getPackageVersion( QStringList() << "chromium-browser" << "firefox" << "MozillaFirefox" );
#else
	QStringList browsers = getPackageVersion( QStringList() << "Google Chrome" << "Firefox" << "Safari" );
#endif
	if ( !browsers.isEmpty() )
		s << "<b>" << tr("Browsers:") << "</b><br />" << browsers.join( "<br />" ) << "<br /><br />";

	diagnosticsText->setHtml( info );

	details = buttonBox->addButton( tr( "More info" ), QDialogButtonBox::HelpRole );
}

QString DiagnosticsDialog::getRegistry( const QString & ) const
{ return QString(); }

QStringList DiagnosticsDialog::getPackageVersion( const QStringList &list, bool returnPackageName ) const
{
	QStringList ret;
#ifdef Q_OS_LINUX
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

	Q_FOREACH( const QString &package, list )
	{
		p.start( cmd, QStringList() << params << package );
		p.waitForFinished();
		if ( p.exitCode() )
			continue;
		QByteArray result = p.readAll();
		if ( !result.isEmpty() )
			ret << (returnPackageName ? package + " " + result : result);
		p.close();
	}
#else
	Q_FOREACH( const QString &package, list )
	{
		QStringList params = QStringList() << "read";
		if( QFile::exists( "/Applications/" + package + ".app/Contents/Info.plist" ) )
			params << "/Applications/" + package + ".app/Contents/Info" << "CFBundleShortVersionString";
		else if( QFile::exists( "/var/db/receipts/ee.sk.idcard." + package + ".plist" ) )
			params << "/var/db/receipts/ee.sk.idcard." + package << "PackageVersion";
		else if( QFile::exists( "/Library/Receipts/" + package + ".pkg/Contents/Info.plist" ) )
			params << "/Library/Receipts/" + package + ".pkg/Contents/Info" << "CFBundleShortVersionString";
		else
			continue;

		QByteArray result = runProcess( "defaults", params );
		if ( !result.isEmpty() )
			ret << (returnPackageName ? package + " "  + result : result);
	}
#endif

	return ret;
}

QString DiagnosticsDialog::getProcessor() const
{
#ifdef Q_OS_LINUX
	return runProcess( "sh -c \"cat /proc/cpuinfo | grep -m 1 model\\ name\"" );
#else
	QString result = runProcess( "system_profiler", QStringList() << "SPHardwareDataType" );
	QRegExp reg( "Processor Name:(\\s*)(.*)\n.*Processor Speed:(\\s*)(.*)\n" );
	reg.setMinimal( true );
	return reg.indexIn( result ) != -1 ? reg.cap( 2 ) + " (" + reg.cap( 4 ) + ")" : "";
#endif
}

QString DiagnosticsDialog::getUserRights() const
{ return QString(); }

bool DiagnosticsDialog::isPCSCRunning()
{
#ifdef Q_OS_LINUX
	QByteArray result = runProcess( "pidof", QStringList() << "pcscd" );
#else
	QByteArray result = runProcess( "sh -c \"ps ax | grep -v grep | grep pcscd\"" );
#endif
	return !result.trimmed().isEmpty();
}

void DiagnosticsDialog::showDetails()
{
	QProgressDialog box( tr( "Generating diagnostics\n\nPlease wait..." ), QString(), 0, 0, qApp->activeWindow() );
	box.setWindowTitle( windowTitle() );
	box.setWindowFlags( (box.windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint );
	if( QProgressBar *bar = box.findChild<QProgressBar*>() )
		bar->setVisible( false );
	box.open();

	QApplication::processEvents();

	QString ret;
	QString cmd = QString::fromUtf8( runProcess( "opensc-tool", QStringList() << "-la" ) );
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("OpenSC tool:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";

	QApplication::processEvents();

	QStringList list;
#if defined(PKCS11_MODULE)
	list << QString("--module=%1").arg( PKCS11_MODULE );
#endif
	list << "-T";

	cmd = QString::fromUtf8( runProcess( "pkcs11-tool", list ) );
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("PKCS11 tool:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";

#ifdef Q_OS_LINUX
	cmd = runProcess( "lsusb" );
#else
	cmd = runProcess( "system_profiler", QStringList() << "SPUSBDataType" );
#endif
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("USB info:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";

	if ( !ret.isEmpty() )
		diagnosticsText->append( ret );

	details->setDisabled( true );
}
