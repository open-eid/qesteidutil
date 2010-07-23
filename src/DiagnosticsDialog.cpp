/*
 * QEstEidUtil
 *
 * Copyright (C) 2009 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009 Raul Metsma <raul@innovaatik.ee>
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
#include <QMessageBox>
#include <QTextStream>

#if defined(Q_OS_WIN32)
#include <Windows.h>
#elif defined(Q_OS_LINUX)
#include <QProcess>
#elif defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif

DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QString info;
	QTextStream s( &info );

	s << "<b>" << tr("ID-card utility version:") << "</b> ";
	s << QCoreApplication::applicationVersion() << "<br />";

	s << "<b>" << tr("OS:") << "</b> ";
#if defined(Q_OS_WIN32)
	switch( QSysInfo::WindowsVersion )
	{
	case QSysInfo::WV_2000: s << "Windows 2000"; break;
	case QSysInfo::WV_XP: s << "Windows XP"; break;
	case QSysInfo::WV_2003: s << "Windows 2003"; break;
	case QSysInfo::WV_VISTA: s << "Windows Vista"; break;
	case QSysInfo::WV_WINDOWS7: s << "Windows 7"; break;
	default: s << "Unknown version (" << QSysInfo::WindowsVersion << ")";
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
#elif defined(Q_OS_MAC)
	s << getLibVersion( "PCSC" ) << "<br />";
#elif defined(Q_OS_LINUX)
	s << getLibVersion( "pcsclite" ) << "<br />";
	s << getLibVersion( "ssl" ) << "<br />";
	s << getLibVersion( "crypto" ) << "<br />";
#endif
	s << getLibVersion( "opensc-pkcs11" ) << "<br />";
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
