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
#include <smartcardpp/DynamicLibrary.h>

#include <QDialogButtonBox>
#include <QLibrary>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QTextStream>

#include <Windows.h>

static QString getLibVersion( const QString &libraries )
{
	QStringList libs = libraries.split( "|" );
	QString resultFound, resultNotFound;
	Q_FOREACH( const QString &lib, libs )
	{
		try
		{
			QString ver = QString::fromStdString( DynamicLibrary( lib.toLatin1() ).getVersionStr() );
			resultFound += QString( "%1 (%2)" )
				.arg( lib )
				.arg( ver == "missing" ? DiagnosticsDialog::tr("failed to get version info") : ver );
		}
		catch( const std::runtime_error &e )
		{
			QString tmp = QString("%1 - %2")
				.arg( lib )
				.arg( QString( e.what() ).contains( "missing" ) ?
					DiagnosticsDialog::tr("failed to get version info") : DiagnosticsDialog::tr("library not found") );
			if ( !resultNotFound.isEmpty() )
				resultNotFound += "<br />";
			resultNotFound += tmp;
		}
	}
	return !resultFound.isEmpty() ? resultFound : resultNotFound;
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
	QString locale = (language == QLocale::C ? "English/United States" : QLocale::languageToString( language ) );
	CPINFOEX CPInfoEx;
	if ( GetCPInfoEx( GetConsoleCP(), 0, &CPInfoEx ) != 0 )
		locale.append( " - " ).append( CPInfoEx.CodePageName );
	s << locale << "<br />";
	s << "<b>" << tr("User rights: ") << "</b>" << getUserRights() << "<br />";

	QString base = getRegistry( "Eesti ID kaardi tarkvara" );
	if ( !base.isEmpty() )
		s << "<b>" << tr("Base version:") << "</b> " << base << "<br />";
	s << "<b>" << tr("ID-card utility version:") << "</b> "<< QCoreApplication::applicationVersion() << "<br />";

	s << "<b>" << tr("OS:") << "</b> " << Common::applicationOs() << "<br />";
	s << "<b>" << tr("CPU:") << "</b> " << getProcessor() << "<br /><br />";

	s << "<b>" << tr("Library paths:") << "</b> " << QCoreApplication::libraryPaths().join( ";" ) << "<br />";

	s << "<b>" << tr("Libraries") << ":</b><br />";
	s << getLibVersion( "digidoc") << "<br />";
	s << getLibVersion( "digidocpp") << "<br />";
	s << getLibVersion( "advapi32") << "<br />";
	s << getLibVersion( "crypt32") << "<br />";
	s << getLibVersion( "winscard") << "<br />";
	s << getLibVersion( "esteidcsp|esteidcm") << "<br />";
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

	details = buttonBox->addButton( tr( "More info" ), QDialogButtonBox::HelpRole );
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

QStringList DiagnosticsDialog::getPackageVersion( const QStringList &list, bool returnPackageName ) const
{ return QStringList(); }

QString DiagnosticsDialog::getProcessor() const
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return QString::number( sysinfo.dwProcessorType );
}

QString DiagnosticsDialog::getUserRights() const
{
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup = NULL;
	HANDLE hToken = NULL;
	DWORD dwIndex, dwLength = 0;
	PTOKEN_GROUPS pGroup = NULL;
	QString rights = tr( "User" );

	if ( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ) )
	{
		if ( GetLastError() != ERROR_NO_TOKEN )
			return tr( "Unknown - error %1" ).arg( GetLastError() );
		if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
			return tr( "Unknown - error %1" ).arg( GetLastError() );
	}
	if ( !GetTokenInformation( hToken, TokenGroups, NULL, dwLength, &dwLength ) )
	{
		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			return tr( "Unknown - error %1" ).arg( GetLastError() );
	}
	pGroup = (PTOKEN_GROUPS)GlobalAlloc( GPTR, dwLength );

	if ( !GetTokenInformation( hToken, TokenGroups, pGroup, dwLength, &dwLength ) )
	{
		if ( pGroup )
			GlobalFree( pGroup );
		return tr( "Unknown - error %1" ).arg( GetLastError() );;
	}

	if( AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
									0, 0, 0, 0, 0, 0, &AdministratorsGroup ) )
	{
		for ( dwIndex = 0; dwIndex < pGroup->GroupCount; dwIndex++ )
		{
			if ( EqualSid( AdministratorsGroup, pGroup->Groups[dwIndex].Sid ) )
			{
				rights = tr( "Administrator" );
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

bool DiagnosticsDialog::isPCSCRunning()
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

	QStringList list;
#if defined(PKCS11_MODULE)
	list << QString("--module=%1").arg( PKCS11_MODULE );
#endif
	list << "-T";
	p.start( "pkcs11-tool", list );
	p.waitForFinished();
	cmd = QString::fromUtf8( p.readAll() );
	if ( !cmd.isEmpty() )
		ret += "<b>" + tr("PKCS11 tool:") + "</b><br/> " + cmd.replace( "\n", "<br />" ) + "<br />";

	if ( !ret.isEmpty() )
		diagnosticsText->append( ret );

	details->setDisabled( true );
}
