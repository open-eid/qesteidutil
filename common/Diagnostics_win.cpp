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

#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <qt_windows.h>

static QString getUserRights()
{
	HANDLE hToken = 0;
	if ( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ) )
	{
		if ( GetLastError() != ERROR_NO_TOKEN )
			return Diagnostics::tr( "Unknown - error %1" ).arg( GetLastError() );
		if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
			return Diagnostics::tr( "Unknown - error %1" ).arg( GetLastError() );
	}

	DWORD dwLength = 0;
	if ( !GetTokenInformation( hToken, TokenGroups, 0, dwLength, &dwLength ) )
	{
		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			return Diagnostics::tr( "Unknown - error %1" ).arg( GetLastError() );
	}

	PTOKEN_GROUPS pGroup = (PTOKEN_GROUPS)GlobalAlloc( GPTR, dwLength );
	if ( !GetTokenInformation( hToken, TokenGroups, pGroup, dwLength, &dwLength ) )
	{
		if ( pGroup )
			GlobalFree( pGroup );
		return Diagnostics::tr( "Unknown - error %1" ).arg( GetLastError() );;
	}

	QString rights = Diagnostics::tr( "User" );
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup = 0;
	if( AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup ) )
	{
		for ( DWORD dwIndex = 0; dwIndex < pGroup->GroupCount; dwIndex++ )
		{
			if ( EqualSid( AdministratorsGroup, pGroup->Groups[dwIndex].Sid ) )
			{
				rights = Diagnostics::tr( "Administrator" );
				break;
			}
		}
	}

	if ( AdministratorsGroup )
		FreeSid( AdministratorsGroup );
	if ( pGroup )
		GlobalFree( pGroup );

	return rights;
}

void Diagnostics::run()
{
	QString info;
	QTextStream s( &info );

	s << "<b>" << tr("Locale (time-, number format / codepage):") << "</b> ";
	QLocale::Language language = QLocale::system().language();
	QString locale = (language == QLocale::C ? "English/United States" : QLocale::languageToString( language ) );
	CPINFOEX CPInfoEx;
	if( GetCPInfoEx( GetConsoleCP(), 0, &CPInfoEx ) != 0 )
		locale.append( " / " ).append( QString( (QChar*)CPInfoEx.CodePageName ) );
	s << locale << "<br />";
	emit update( info );
	info.clear();

	s << "<b>" << tr("User rights: ") << "</b>" << getUserRights() << "<br />";
	emit update( info );
	info.clear();

	QStringList base = Common::packages( QStringList() << "Eesti ID-kaardi tarkvara", false );
	if( !base.isEmpty() )
		s << "<b>" << tr("Base version:") << "</b> " << base.join( "<br />" ) << "<br />";
	s << "<b>" << tr("Application version:") << "</b> "<< QCoreApplication::applicationVersion()
#ifdef INTERNATIONAL
		<< " INTERNATIONAL"
#endif
		<< "<br />";
	emit update( info );
	info.clear();

	s << "<b>" << tr("OS:") << "</b> " << Common::applicationOs() << "<br />";
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	s << "<b>" << tr("CPU:") << "</b> " << QString::number( sysinfo.dwProcessorType ) << "<br /><br />";
	emit update( info );
	info.clear();

	s << "<b>" << tr("Arguments:") << "</b> " << qApp->arguments().join(" ") << "<br />";
	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";

	s << "<b>" << tr("Libraries") << ":</b><br />" << "QT (" << qVersion() << ")<br />";
	Q_FOREACH( const QString &lib, QStringList()
			<< "digidoc" << "digidocpp" << "qdigidocclient.exe" << "qesteidutil.exe" << "id-updater.exe"
			<< "esteidcsp" << "esteidcm" << "EsteidShellExtension" << "esteid-plugin-ie"
			<< "npesteid-firefox-plugin" << "opensc-pkcs11" << "esteid-pkcs11"
			<< "libeay32" << "ssleay32" << "advapi32" << "crypt32" << "winscard" )
	{
		DWORD infoHandle = 0;
		LONG sz = GetFileVersionInfoSize( LPCWSTR(lib.utf16()), &infoHandle );
		if( !sz )
			continue;
		QByteArray data( sz * 2, 0 );
		if( !GetFileVersionInfoW( LPCWSTR(lib.utf16()), 0, sz, data.data() ) )
			continue;
		VS_FIXEDFILEINFO *info = 0;
		UINT len = 0;
		if( !VerQueryValueW( data.constData(), L"\\", (LPVOID*)&info, &len ) )
			continue;
		s << QString( "%1 (%2.%3.%4.%5)" ).arg( lib )
			.arg( HIWORD(info->dwFileVersionMS) )
			.arg( LOWORD(info->dwFileVersionMS) )
			.arg( HIWORD(info->dwFileVersionLS) )
			.arg( LOWORD(info->dwFileVersionLS) ) << "<br />";
	}
	s << "<br />";
	emit update( info );
	info.clear();

	enum {
		Running,
		Stopped,
		NotFound
	} atrfiltr = NotFound, certprop = NotFound;
	if( SC_HANDLE h = OpenSCManager( 0, 0, SC_MANAGER_CONNECT ) )
	{
		if( SC_HANDLE s = OpenService( h, L"atrfiltr", SERVICE_QUERY_STATUS ) )
		{
			SERVICE_STATUS status;
			QueryServiceStatus( s, &status );
			atrfiltr = (status.dwCurrentState == SERVICE_RUNNING) ? Running : Stopped;
			CloseServiceHandle( s );
		}
		if( SC_HANDLE s = OpenService( h, L"CertPropSvc", SERVICE_QUERY_STATUS ))
		{
			SERVICE_STATUS status;
			QueryServiceStatus( s, &status );
			certprop = (status.dwCurrentState == SERVICE_RUNNING) ? Running : Stopped;
			CloseServiceHandle( s );
		}
		CloseServiceHandle( h );
	}
	s << "<br /><b>" << tr("ATRfiltr service status: ") << "</b>" << " ";
	switch( atrfiltr )
	{
	case NotFound: s << tr("Not found"); break;
	case Stopped: s << tr("Not running"); break;
	case Running: s << tr("Running"); break;
	}
	s << "<br /><b>" << tr("Certificate Propagation service status: ") << "</b>" << " ";
	switch( certprop )
	{
	case NotFound: s << tr("Not found"); break;
	case Stopped: s << tr("Not running"); break;
	case Running: s << tr("Running"); break;
	}
	s << "<br />";

	getReaderInfo( s );
	emit update( info );
	info.clear();

	QStringList browsers = Common::packages( QStringList() << "Firefox" << "Google Chrome" );
	QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Internet Explorer", QSettings::NativeFormat );
	browsers << QString( "Internet Explorer (%1)" ).arg(
		reg.value("svcVersion", reg.value( "Version" ) ).toString() );
	s << "<br /><br /><b>" << tr("Browsers:") << "</b><br />" << browsers.join( "<br />" ) << "<br /><br />";
	emit update( info );
	info.clear();
}
