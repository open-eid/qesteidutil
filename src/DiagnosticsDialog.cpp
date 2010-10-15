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

#include "smartcardpp/common.h"
#include "smartcardpp/DynamicLibrary.h"
#include "smartcardpp/SmartCardManager.h"
#include "smartcardpp/esteid/EstEidCard.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QImageReader>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QTextStream>

#if defined(Q_OS_WIN32)
#include <Windows.h>
#elif defined(Q_OS_LINUX)
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

#if defined(Q_OS_LINUX)
	QString package = getPackageVersion( QStringList() << "estonianidcard", false );
	QString utility = getPackageVersion( QStringList() << "qesteidutil", false );
	if ( !package.isEmpty() )
	{
		s << "<b>" << tr("ID-card package version:") << "</b> ";
		s << package << "<br />";
	}
	if ( !utility.isEmpty() )
	{
		s << "<b>" << tr("ID-card utility version:") << "</b> ";
		s << utility << "<br />";
	}
#else
	s << "<b>" << tr("ID-card utility version:") << "</b> ";
	s << QCoreApplication::applicationVersion() << "<br />";
#endif

	s << "<b>" << tr("OS:") << "</b> ";
#if defined(Q_OS_WIN32)
	OSVERSIONINFOEX osvi;
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	if( !GetVersionEx( (OSVERSIONINFO *) &osvi) )
	{
		switch( QSysInfo::WindowsVersion )
		{
		case QSysInfo::WV_2000: s << "Windows 2000"; break;
		case QSysInfo::WV_XP: s << "Windows XP"; break;
		case QSysInfo::WV_2003: s << "Windows 2003"; break;
		case QSysInfo::WV_VISTA: s << "Windows Vista"; break;
		case QSysInfo::WV_WINDOWS7: s << "Windows 7"; break;
		default: s << "Unknown version (" << QSysInfo::WindowsVersion << ")";
		}
	} else {
		switch( osvi.dwMajorVersion )
		{
		case 5:
			{
				switch( osvi.dwMinorVersion )
				{
				case 0:
					s << "Windows 2000 ";
					s << ( osvi.wProductType == VER_NT_WORKSTATION ? "Professional" : "Server" );
					break;
				case 1:
					s << "Windows XP ";
					s << ( osvi.wSuiteMask & VER_SUITE_PERSONAL ? "Home" : "Professional" );
					break;
				case 2:
					if ( GetSystemMetrics( SM_SERVERR2 ) )
						s << "Windows Server 2003 R2";
					else if ( osvi.wProductType == VER_NT_WORKSTATION )
						s << "Windows XP Professional";
					else
						s << "Windows Server 2003";
					break;
				}
				break;
			}	
		case 6:
			{
				switch( osvi.dwMinorVersion )
				{
					case 0: s << ( osvi.wProductType == VER_NT_WORKSTATION ? "Windows Vista" : "Windows Server 2008" ); break;
					case 1: s << ( osvi.wProductType == VER_NT_WORKSTATION ? "Windows 7" : "Windows Server 2008 R2" ); break;
				}
				break;
			}
		default: s << "Unknown version (" << QSysInfo::WindowsVersion << ")";
		}
	}
#elif defined(Q_OS_LINUX)
	QProcess p;
	p.start( "lsb_release", QStringList() << "-s" << "-d" );
	p.waitForReadyRead();
	s << p.readAll();
#elif defined(Q_OS_MAC)
	SInt32 major, minor, bugfix;
	
	if(Gestalt(gestaltSystemVersionMajor, &major) == noErr && Gestalt(gestaltSystemVersionMinor, &minor) == noErr && Gestalt(gestaltSystemVersionBugFix, &bugfix) == noErr) {
		s << "Mac OS " << major << "." << minor << "." << bugfix;
	} else {
		s << "Mac OS 10.3";
	}
#endif

#if defined(Q_OS_WIN)
	QString bits = "32";
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	BOOL bIsWow64 = false;
	//check if kernel32 supports this function
	fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( GetModuleHandle(TEXT("kernel32")), "IsWow64Process" );
	if ( fnIsWow64Process != NULL )
	{
		if ( fnIsWow64Process( GetCurrentProcess(), &bIsWow64 ) )
			if ( bIsWow64 )
				bits = "64";
	} else {
		SYSTEM_INFO sysInfo;
		GetSystemInfo( &sysInfo );
		if ( sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
			bits = "64";
	}
	s << " (" << bits << ")<br /><br />";
#else
	s << " (" << QSysInfo::WordSize << ")<br /><br />";
#endif

	s << "<b>" << tr("Library paths:") << "</b> ";
	s << QCoreApplication::libraryPaths().join( ";" ) << "<br />";

	s << "<b>" << tr("Libraries") << "</b><br />";
#if defined(Q_OS_WIN32)
	s << getLibVersion( "advapi32") << "<br />";
	s << getLibVersion( "libeay32" ) << "<br />";
	s << getLibVersion( "ssleay32" ) << "<br />";
	s << getLibVersion( "opensc-pkcs11" ) << "<br />";
#elif defined(Q_OS_MAC)
	s << getLibVersion( "PCSC" ) << "<br />";
#elif defined(Q_OS_LINUX)
	s << getPackageVersion( QStringList() << "openssl" << "libpcsclite1" << "opensc" );
#endif
	s << "QT (" << qVersion() << ")<br />";
	s << "<br />";

#if defined(Q_OS_WIN32)
	s << "<b>" << tr("Smart Card service status: ") << "</b>";
#else
	s << "<b>" << tr("PCSC service status: ") << "</b>";
#endif
	s << " " << (isPCSCRunning() ? tr("Running") : tr("Not running"));
	s << "<br /><br />";

	s << "<b>" << tr("Card readers") << "</b><br />";
	s << getReaderInfo();
	s << "<br />";

	diagnosticsText->setHtml( info );
}

QString DiagnosticsDialog::getLibVersion( const QString &lib ) const
{
	try
	{
		return QString( "%1 (%2)" ).arg( lib )
			.arg( QString::fromStdString( DynamicLibrary( lib.toLatin1() ).getVersionStr() ) );
	}
	catch( const std::runtime_error & )
	{ return tr("%1 - failed to get version info").arg( lib ); }
}

QString DiagnosticsDialog::getPackageVersion( const QStringList &list, bool returnPackageName ) const
{
	QString ret;
#ifndef Q_OS_WIN32
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
		p.waitForReadyRead();
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

QString DiagnosticsDialog::getReaderInfo() const
{
	QString d;
	QTextStream s( &d );

	QHash<QString,QString> readers;
	SmartCardManager *m = 0;
	QString reader;
	try {
		m = new SmartCardManager();
		int readersCount = m->getReaderCount( true );
		for( int i = 0; i < readersCount; i++ )
		{
			reader = QString::fromStdString( m->getReaderName( i ) );
			if ( !QString::fromStdString( m->getReaderState( i ) ).contains( "EMPTY" ) )
			{
				EstEidCard card( *m );
				card.connect( i );
				readers[reader] = tr( "ID - %1" ).arg( QString::fromStdString( card.readCardID() ) );
			}
			else
				readers[reader] = "";
		}
	} catch( const std::runtime_error &e ) {
		readers[reader] = tr("Error reading card data:") + e.what();
	}
	delete m;

	for( QHash<QString,QString>::const_iterator i = readers.constBegin();
		i != readers.constEnd(); ++i )
	{
		s << "* " << i.key();
		if( !i.value().isEmpty() )
			s << "<p style='margin-left: 10px; margin-top: 0px; margin-bottom: 0px; margin-right: 0px;'>" << i.value() << "</p>";
		else
			s << "<br />";
	}

	return d;
}

#if defined(Q_OS_WIN32)
bool DiagnosticsDialog::isPCSCRunning() const
{
	bool result = false;
	SC_HANDLE h = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( h )
	{
		SC_HANDLE s = OpenService( h, "SCardSvr", SERVICE_QUERY_STATUS );
		if( s )
		{
			SERVICE_STATUS status;
			QueryServiceStatus( s, &status );
			result = (status.dwCurrentState == SERVICE_RUNNING);
			CloseServiceHandle( s );
		}
		CloseServiceHandle( h );
	}
	return result;
}
#elif defined(Q_OS_LINUX)
bool DiagnosticsDialog::isPCSCRunning() const
{
	QProcess p;
	p.start( "pidof", QStringList() << "pcscd" );
	p.waitForFinished();
	return !p.readAll().trimmed().isEmpty();
}
#elif defined(Q_OS_MAC)
bool DiagnosticsDialog::isPCSCRunning() const
{
	QProcess p;
	p.start( "sh -c \"ps ax | grep -v grep | grep pcscd\"" );
	p.waitForFinished();
	return !p.readAll().trimmed().isEmpty();
}
#else
bool DiagnosticsDialog::isPCSCRunning() const { return true; }
#endif

void DiagnosticsDialog::save()
{
	QString filename = QFileDialog::getSaveFileName( this, tr("Save as"), QString( "%1%2qesteidutil_diagnostics.txt" )
		.arg( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) ).arg( QDir::separator() ),
		tr("Text files (*.txt)") );
	if( filename.isEmpty() )
		return;
	QFile f( filename );
	if( f.open( QIODevice::WriteOnly ) )
	{
		QTextStream( &f ) << diagnosticsText->toPlainText();
		f.close();
	}
	else
		QMessageBox::warning( this, tr("Error occured"), tr("Failed write to file!") );
}
