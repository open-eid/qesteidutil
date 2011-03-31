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

#include <smartcardpp/DynamicLibrary.h>

#include <QDialogButtonBox>
#include <QLibrary>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QTextStream>

#include <Windows.h>

DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QString info;
	QTextStream s( &info );

	s << "<b>" << tr("Locale:") << "</b> ";
	QLocale::Language language = QLocale::system().language();
	s << (language == QLocale::C ? "English/United States" : QLocale::languageToString( language ) ) << "<br /><br />";

	QString base = getRegistry( "Eesti ID kaardi tarkvara" );
	if ( !base.isEmpty() )
		s << "<b>" << tr("Base version:") << "</b> " << base << "<br />";
	s << "<b>" << tr("ID-card utility version:") << "</b> "<< QCoreApplication::applicationVersion() << "<br />";

	s << "<b>" << tr("OS:") << "</b> " << getOS() << " (" << getBits() << ")<br />";
	s << "<b>" << tr("CPU:") << "</b> " << getProcessor() << "<br /><br />";

	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";

	s << "<b>" << tr("Libraries") << ":</b><br />";
	s << getLibVersion( "digidoc") << "<br />";
	s << getLibVersion( "digidocpp") << "<br />";
	s << getLibVersion( "advapi32") << "<br />";
	s << getLibVersion( "crypt32") << "<br />";
	s << getLibVersion( "winscard") << "<br />";
	s << getLibVersion( "esteidcsp") << "<br />";
	s << getLibVersion( "esteidcm") << "<br />";
	s << getLibVersion( "libeay32" ) << "<br />";
	s << getLibVersion( "ssleay32" ) << "<br />";
	s << getLibVersion( "opensc-pkcs11" ) << "<br />";
	s << "QT (" << qVersion() << ")<br />" << "<br />";

	s << "<b>" << tr("Smart Card service status: ") << "</b>" << " " << (isPCSCRunning() ? tr("Running") : tr("Not running")) << "<br /><br />";

	s << "<b>" << tr("Card readers") << ":</b><br />" << getReaderInfo() << "<br />";

	QString browsers = getRegistry( "Mozilla|Windows Internet Explorer|Google Chrome" );
	if ( !browsers.isEmpty() )
		s << "<b>" << tr("Browsers:") << "</b><br />" << browsers << "<br /><br />";

	diagnosticsText->setHtml( info );

	buttonBox->addButton( tr( "More info" ), QDialogButtonBox::HelpRole );
}

QString DiagnosticsDialog::getRegistry( const QString &search ) const
{
	QStringList browsers;
	Q_FOREACH( const QString &group, QStringList() << "HKEY_LOCAL_MACHINE" << "HKEY_CURRENT_USER" )
	{
		QSettings s( group + "\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::NativeFormat );
		Q_FOREACH( const QString &key, s.childGroups() )
		{
			QString name = s.value( key + "/DisplayName" ).toString();
			QString version = s.value( key + "/DisplayVersion" ).toString();
			QString type = s.value( key + "/ReleaseType" ).toString();
			if( !type.contains( "Update", Qt::CaseInsensitive ) &&
				name.contains( QRegExp( search, Qt::CaseInsensitive ) ) )
				browsers << QString( "%1 (%2)" ).arg( name, version );
		}
	}
	if ( search.contains( "Internet Explorer" ) && !browsers.join(",").contains( QRegExp( "Windows Internet Explorer", Qt::CaseInsensitive ) ) )
	{
		QSettings s( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Internet Explorer", QSettings::NativeFormat );
		browsers << QString( "Internet Explorer (%2)" ).arg( s.value( "Version" ).toString() );
	}
	return browsers.join( "<br />" );
}

QString DiagnosticsDialog::getBits() const
{
	QString bits = "32";
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	//check if kernel32 supports this function
	QLibrary lib( "kernel32" );
	BOOL bIsWow64 = false;
	if( LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)lib.resolve( "IsWow64Process" ) )
	{
		if( fnIsWow64Process( GetCurrentProcess(), &bIsWow64 ) && bIsWow64 )
			bits = "64";
	}
	else
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo( &sysInfo );
		if ( sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
			bits = "64";
	}
	return bits;
}

QString DiagnosticsDialog::getLibVersion( const QString &lib ) const
{
	try
	{
		QString ver = QString::fromStdString( DynamicLibrary( lib.toLatin1() ).getVersionStr() );
		return QString( "%1 (%2)" )
					.arg( lib )
					.arg( ver == "missing" ? tr( "failed to get version info" ) : ver );
	} catch( const std::runtime_error &e )
	{
		return QString("%1 - %2")
				.arg( lib )
				.arg( QString( e.what() ).contains( "missing" ) ?  tr( "failed to get version info" ) : tr( "library not found" ) );
	}
}

QString DiagnosticsDialog::getOS() const
{
	OSVERSIONINFOEX osvi;
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	if( !GetVersionEx( (OSVERSIONINFO *)&osvi ) )
	{
		switch( QSysInfo::WindowsVersion )
		{
		case QSysInfo::WV_2000: return "Windows 2000";
		case QSysInfo::WV_XP: return "Windows XP";
		case QSysInfo::WV_2003: return "Windows 2003";
		case QSysInfo::WV_VISTA: return "Windows Vista";
		case QSysInfo::WV_WINDOWS7: return "Windows 7";
		default: return QString( "%1 (%2)" ).arg( tr("Unknown version"), QSysInfo::WindowsVersion );
		}
	}
	else
	{
		switch( osvi.dwMajorVersion )
		{
		case 5:
			switch( osvi.dwMinorVersion )
			{
			case 0: return QString( "Windows 2000 %1" ).arg( osvi.wProductType == VER_NT_WORKSTATION ? "Professional" : "Server" );
			case 1: return QString( "Windows XP %1" ).arg( osvi.wSuiteMask & VER_SUITE_PERSONAL ? "Home" : "Professional" );
			case 2:
				if ( GetSystemMetrics( SM_SERVERR2 ) )
					return "Windows Server 2003 R2";
				else if ( osvi.wProductType == VER_NT_WORKSTATION )
					return "Windows XP Professional";
				else
					return "Windows Server 2003";
			}
			break;
		case 6:
			switch( osvi.dwMinorVersion )
			{
			case 0: return osvi.wProductType == VER_NT_WORKSTATION ? "Windows Vista" : "Windows Server 2008";
			case 1: return osvi.wProductType == VER_NT_WORKSTATION ? "Windows 7" : "Windows Server 2008 R2";
			}
			break;
		default: return QString( "%1 (%2)" ).arg( tr("Unknown version"), QSysInfo::WindowsVersion );
		}
	}
	return tr("Unknown version");
}

QStringList DiagnosticsDialog::getPackageVersion( const QStringList &list, bool returnPackageName ) const
{ return QStringList(); }

QString DiagnosticsDialog::getProcessor() const
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return QString::number( sysinfo.dwProcessorType );
}

bool DiagnosticsDialog::isPCSCRunning() const
{
	SC_HANDLE h = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !h )
		return false;

	bool result = false;
	if( SC_HANDLE s = OpenService( h, "SCardSvr", SERVICE_QUERY_STATUS ) )
	{
		SERVICE_STATUS status;
		QueryServiceStatus( s, &status );
		result = (status.dwCurrentState == SERVICE_RUNNING);
		CloseServiceHandle( s );
	}
	CloseServiceHandle( h );
	return result;
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
	QProcess p;
	p.start( "opensc-tool", QStringList() << "-la" );
	p.waitForFinished();
	QString cmd = QString::fromUtf8( p.readAll() );
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("OpenSC tool:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";
	
	QApplication::processEvents();

	p.start( "pkcs11-tool", QStringList() << "-T" );
	p.waitForFinished();
	cmd = QString::fromUtf8( p.readAll() );
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("PKCS11 tool:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";

	if ( !ret.isEmpty() )
		diagnosticsText->append( ret );
}
