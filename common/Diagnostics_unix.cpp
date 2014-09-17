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

#include "Diagnostics.h"

#include "Common.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QProcess>

#include <sys/utsname.h>

void Diagnostics::run()
{
	QString info;
	QTextStream s( &info );

	QLocale::Language language = QLocale::system().language();
	QString ctype = QProcessEnvironment::systemEnvironment().value( "LC_CTYPE",
		QProcessEnvironment::systemEnvironment().value( "LANG" ) );
	s << "<b>" << tr("Locale (time-, number format / codepage):") << "</b> "
		<< (language == QLocale::C ? "English/United States" : QLocale::languageToString( language )) << " / "
		<< ctype << "<br /><br />";
	emit update( info );
	info.clear();

#ifndef Q_OS_MAC
	QStringList package = Common::packages( { "estonianidcard" }, false );
	if( !package.isEmpty() )
		s << "<b>" << tr("Base version:") << "</b> " << package.first() << "<br />";
#endif
	s << "<b>" << tr("Application version:") << "</b> " << QCoreApplication::applicationVersion()
#ifdef INTERNATIONAL
		<< " INTERNATIONAL"
#endif
		<< "<br />";
	emit update( info );
	info.clear();

	s << "<b>" << tr("OS:") << "</b> " << Common::applicationOs() << "<br />";
#ifndef Q_OS_MAC
	s << "<b>" << tr("CPU:") << "</b> ";
	QFile f( "/proc/cpuinfo" );
	if( f.open( QFile::ReadOnly ) )
	{
		QRegExp rx( "model name.*\\: (.*)\n" );
		rx.setMinimal( true );
		int pos = rx.indexIn( QString::fromLocal8Bit( f.readAll() ) );
		if( pos != -1 )
			s << rx.cap( 1 );
	}
#endif
	s << "<br />";
	struct utsname unameData;
	uname(&unameData);
	s << "<b>" << tr("Kernel:") << "</b> "
	  << unameData.sysname << " " << unameData.release << " "
	  << unameData.version << " " << unameData.machine << "<br /><br />";
	emit update( info );
	info.clear();

	s << "<b>" << tr("Arguments:") << "</b> " << qApp->arguments().join(" ") << "<br />";
	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";
	s << "<b>" << tr("Libraries") << ":</b><br />";
#ifdef Q_OS_MAC
	s << Common::packages( { "libdigidoc", "digidocpp" } ).join( "<br />" ) << "<br />";
#else
	s << Common::packages( { "libdigidoc2", "libdigidocpp0", "qdigidoc", "qesteidutil",
		"esteidpkcs11loader", "esteidfirefoxplugin", "chrome-token-signing", "openssl",
		"libpcsclite1", "pcsc-lite", "opensc", "esteid-pkcs11" } ).join( "<br />" ) << "<br />";
#endif
	s << "QT (" << qVersion() << ")" << "<br /><br />";
	emit update( info );
	info.clear();

	getReaderInfo( s );
	emit update( info );
	info.clear();

#ifndef Q_OS_MAC
	QStringList browsers = Common::packages( { "chromium-browser", "firefox", "MozillaFirefox" } );
	if( !browsers.isEmpty() )
		s << "<br /><br /><b>" << tr("Browsers:") << "</b><br />" << browsers.join( "<br />" ) << "<br /><br />";
	emit update( info );
	info.clear();

	QProcess p;
	p.start( "lsusb" );
	p.waitForFinished();
	QString cmd = QString::fromLocal8Bit( p.readAll() );
	if( !cmd.isEmpty() )
		s << "<b>" << tr("USB info:") << "</b><br/> " << cmd.replace( "\n", "<br />" ) << "<br />";
	emit update( info );
	info.clear();
#endif
}
