/*
 * QEstEidCommon
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

#include "Common.h"

#include "SslCertificate.h"
#include "TokenData.h"
#include "Settings.h"

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QPalette>
#include <QProcess>
#include <QTextStream>
#include <QUrl>

#include <stdlib.h>

#if defined(Q_OS_WIN)
#include <QLibrary>

#include <windows.h>
#include <mapi.h>
#elif defined(Q_OS_MAC)
#ifdef USE_SECURITY
#include <Authorization.h>
#endif
#include <Carbon/Carbon.h>
#endif

Common::Common( int &argc, char **argv )
:	QtSingleApplication( argc, argv )
{
#if defined(Q_OS_WIN)
	AllowSetForegroundWindow( ASFW_ANY );
#elif defined(Q_OS_MAC)
	macEvents = 0;
#ifndef QT_MAC_USE_COCOA
	initMacEvents();
#endif
#endif
}

Common::~Common()
{
#if defined(Q_OS_MAC)
	deinitMacEvents();
#endif
}

QString Common::applicationOs()
{
#if defined(Q_OS_LINUX)
	QProcess p;
	p.start( "lsb_release", QStringList() << "-s" << "-d" );
	p.waitForFinished();
	return QString::fromLocal8Bit( p.readAll().trimmed() );
#elif defined(Q_OS_MAC)
	SInt32 major, minor, bugfix;
	if( Gestalt(gestaltSystemVersionMajor, &major) == noErr &&
			Gestalt(gestaltSystemVersionMinor, &minor) == noErr &&
			Gestalt(gestaltSystemVersionBugFix, &bugfix) == noErr )
		return QString( "Mac OS %1.%2.%3 (%4)" ).arg( major ).arg( minor ).arg( bugfix ).arg( QSysInfo::WordSize );
	else
		return QString( "Mac OS 10.3 (%1)" ).arg( QSysInfo::WordSize );
#elif defined(Q_OS_WIN)
	QString os;
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	ZeroMemory( &si, sizeof(SYSTEM_INFO) );

	bool is64bit = false;
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	if( GetVersionEx( (OSVERSIONINFO *)&osvi ) )
	{
		typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
		if( PGNSI pGNSI = PGNSI( QLibrary( "kernel32" ).resolve( "GetNativeSystemInfo" ) ) )
			pGNSI( &si );
		else
			GetSystemInfo( &si );
		switch( osvi.dwMajorVersion )
		{
		case 5:
			switch( osvi.dwMinorVersion )
			{
			case 0:
				os = QString( "Windows 2000 %1" ).arg( osvi.wProductType == VER_NT_WORKSTATION ? "Professional" : "Server" );
				break;
			case 1:
				os = QString( "Windows XP %1" ).arg( osvi.wSuiteMask & VER_SUITE_PERSONAL ? "Home" : "Professional" );
				break;
			case 2:
				if( GetSystemMetrics( SM_SERVERR2 ) )
					os = "Windows Server 2003 R2";
				else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
				{
					is64bit = true;
					os = "Windows XP Professional";
				} else {
					os = "Windows Server 2003";
					if ( osvi.wProductType != VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
						is64bit = true;
				}
				break;
			default: break;
			}
			break;
		case 6:
			switch( osvi.dwMinorVersion )
			{
			case 0:
				os = osvi.wProductType == VER_NT_WORKSTATION ? "Windows Vista" : "Windows Server 2008";
				break;
			case 1:
				os = osvi.wProductType == VER_NT_WORKSTATION ? "Windows 7" : "Windows Server 2008 R2";
				break;
			}
			if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
				is64bit = true;
			break;
		default: break;
		}
	}
	else
	{
		switch( QSysInfo::WindowsVersion )
		{
		case QSysInfo::WV_2000: os = "Windows 2000"; break;
		case QSysInfo::WV_XP: os = "Windows XP"; break;
		case QSysInfo::WV_2003: os = "Windows 2003"; break;
		case QSysInfo::WV_VISTA: os = "Windows Vista"; break;
		case QSysInfo::WV_WINDOWS7: os = "Windows 7"; break;
		default: break;
		}
	}

	if( !os.isEmpty() )
	{
		if ( osvi.szCSDVersion > 0 )
			os.append( " " ).append( osvi.szCSDVersion );

		return QString( "%1 (%2 bit)" ).arg( os ).arg( is64bit ? "64" : "32" );
	}
#endif

	return tr("Unknown OS");
}

bool Common::canWrite( const QString &filename )
{
	QFileInfo i( filename );
#ifdef Q_OS_WIN
	if( i.isFile() )
		return QFile( filename ).open( QFile::WriteOnly|QFile::Append );
	QFile f( i.absolutePath().append( "/.XXXXXX" ) );
	bool result = f.open( QFile::WriteOnly );
	f.remove();
	return result;
#else
	return i.isFile() ? i.isWritable() : QFileInfo( i.absolutePath() ).isWritable();
#endif
}

void Common::browse( const QUrl &url )
{
	QUrl u = url;
	u.setScheme( "file" );
#if defined(Q_OS_WIN)
	if( QProcess::startDetached( "explorer", QStringList() << "/select," <<
		QDir::toNativeSeparators( u.toLocalFile() ) ) )
		return;
#elif defined(Q_OS_MAC)
	QProcess p;
	p.start( "/usr/bin/osascript", QStringList() << "-" << u.toLocalFile() );
	p.waitForStarted();
	QTextStream s( &p );
	s << "on run argv" << endl
		<< "set vfile to POSIX file (item 1 of argv)" << endl
		<< "tell application \"Finder\"" << endl
		<< "select vfile" << endl
		<< "activate" << endl
		<< "end tell" << endl
		<< "end run" << endl;
	p.closeWriteChannel();
	p.waitForFinished();
	if( p.exitCode() == 0 )
		return;
#endif
	QDesktopServices::openUrl( QUrl::fromLocalFile( QFileInfo( u.toLocalFile() ).absolutePath() ) );
}

QString Common::helpUrl()
{
	QString lang = Settings::language();
	if( lang == "en" ) return "http://support.sk.ee/eng/";
	if( lang == "ru" ) return "http://support.sk.ee/ru/";
	return "http://support.sk.ee";
}

void Common::initDigiDoc()
{
	setStyleSheet(
		"QDialogButtonBox { dialogbuttonbox-buttons-have-icons: 0; }\n" );
	QPalette p = palette();
	p.setBrush( QPalette::Link, QBrush( "#509B00" ) );
	p.setBrush( QPalette::LinkVisited, QBrush( "#509B00" ) );
	setPalette( p );

	qRegisterMetaType<QSslCertificate>("QSslCertificate");
	qRegisterMetaType<TokenData>("TokenData");

	QDesktopServices::setUrlHandler( "browse", this, "browse" );
	QDesktopServices::setUrlHandler( "mailto", this, "mailTo" );
}

bool Common::event( QEvent *e )
{
#ifdef QT_MAC_USE_COCOA
	// Load here because cocoa NSApplication overides events
	if( e->type() == QEvent::ApplicationActivate && !macEvents )
		initMacEvents();
#endif
	return QtSingleApplication::event( e );
}

QString Common::fileSize( quint64 bytes )
{
	const quint64 kb = 1024;
	const quint64 mb = 1024 * kb;
	const quint64 gb = 1024 * mb;
	const quint64 tb = 1024 * gb;
	if( bytes >= tb )
		return QString( "%1 TB" ).arg( qreal(bytes) / tb, 0, 'f', 3 );
	if( bytes >= gb )
		return QString( "%1 GB" ).arg( qreal(bytes) / gb, 0, 'f', 2 );
	if( bytes >= mb )
		return QString( "%1 MB" ).arg( qreal(bytes) / mb, 0, 'f', 1 );
	if( bytes >= kb )
		return QString( "%1 KB" ).arg( bytes / kb );
	return QString( "%1 B" ).arg( bytes );
}

void Common::mailTo( const QUrl &url )
{
#if defined(Q_OS_WIN)
	QString mail = QSettings( "HKEY_CURRENT_USER\\Software\\Clients\\Mail",
		QSettings::NativeFormat ).value( "." ).toString();
	bool utf8 = QSettings( "HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail\\" + mail,
		QSettings::NativeFormat ).value( "SupportUTF8", false ).toBool();

	QString file = url.queryItemValue( "attachment" );
	QString filePath = QDir::toNativeSeparators( file );
	QString fileName = QFileInfo( file ).fileName();
	QByteArray subject = toLocal8Bit( url.queryItemValue( "subject" ), utf8 );

	MapiFileDesc doc[1];
	doc[0].ulReserved = 0; //utf8 ? CP_UTF8 : 0;
	doc[0].flFlags = 0;
	doc[0].nPosition = -1;
	doc[0].lpszPathName = (char*)filePath.utf16();
	doc[0].lpszFileName = (char*)fileName.utf16();
	doc[0].lpFileType = 0;

	// Create message
	MapiMessage message;
	message.ulReserved = utf8 ? CP_UTF8 : 0;
	message.lpszSubject = const_cast<char*>(subject.constData());
	message.lpszNoteText = "";
	message.lpszMessageType = 0;
	message.lpszDateReceived = 0;
	message.lpszConversationID = 0;
	message.flFlags = 0;
	message.lpOriginator = 0;
	message.nRecipCount = 0;
	message.lpRecips = 0;
#if 0
	message.nFileCount = 1;
	message.lpFiles = lpMapiFileDesc(&doc);
#else
	message.nFileCount = 0;
	message.lpFiles = 0;
#endif

	QLibrary lib("mapi32");
	if( LPMAPISENDMAIL mapi = LPMAPISENDMAIL(lib.resolve("MAPISendMail")) )
	{
		switch( mapi( NULL, 0, &message, MAPI_LOGON_UI|MAPI_DIALOG, 0 ) )
		{
		case SUCCESS_SUCCESS:
		case MAPI_E_USER_ABORT:
		case MAPI_E_LOGIN_FAILURE:
			return;
		default: break;
		}
	}
#elif defined(Q_OS_MAC)
	CFURLRef emailUrl = CFURLCreateWithString( kCFAllocatorDefault, CFSTR("mailto:"), 0 );
	CFURLRef appUrl = 0;
	CFStringRef appPath = 0;
	if( LSGetApplicationForURL( emailUrl, kLSRolesAll, NULL, &appUrl ) == noErr )
	{
		appPath = CFURLCopyFileSystemPath( appUrl, kCFURLPOSIXPathStyle );
		CFRelease( appUrl );
	}
	CFRelease( emailUrl );

	if( appPath )
	{
		QProcess p;
		p.start( "/usr/bin/osascript", QStringList() << "-" << url.queryItemValue("attachment") << url.queryItemValue("subject") );
		p.waitForStarted();
		QTextStream s( &p );
		if( CFStringCompare( appPath, CFSTR("/Applications/Mail.app"), 0 ) == kCFCompareEqualTo )
		{
			s << "on run argv" << endl
				<< "set vattachment to (item 1 of argv)" << endl
				<< "set vsubject to (item 2 of argv)" << endl
				<< "tell application \"Mail\"" << endl
				<< "set composeMessage to make new outgoing message at beginning with properties {visible:true}" << endl
				<< "tell composeMessage" << endl
				<< "set subject to vsubject" << endl
				<< "set content to \" \"" << endl
				<< "tell content" << endl
				<< "make new attachment with properties {file name: vattachment} at after the last word of the last paragraph" << endl
				<< "end tell" << endl
				<< "end tell" << endl
				<< "activate" << endl
				<< "end tell" << endl
				<< "end run" << endl;
		}
		else if( CFStringFind( appPath, CFSTR("Entourage"), 0 ).location != kCFNotFound )
		{
			s << "on run argv" << endl
				<< "set vattachment to (item 1 of argv)" << endl
				<< "set vsubject to (item 2 of argv)" << endl
				<< "tell application \"Microsoft Entourage\"" << endl
				<< "set vmessage to make new outgoing message with properties" << endl
				<< "{subject:vsubject, attachments:vattachment}" << endl
				<< "open vmessage" << endl
				<< "activate" << endl
				<< "end tell" << endl
				<< "end run" << endl;
		}
		else if( CFStringFind( appPath, CFSTR("Outlook"), 0 ).location != kCFNotFound )
		{
			s << "on run argv" << endl
				<< "set vattachment to (item 1 of argv)" << endl
				<< "set vsubject to (item 2 of argv)" << endl
				<< "tell application \"Microsoft Outlook\"" << endl
				<< "activate" << endl
				<< "set vmessage to make new outgoing message with properties {subject:vsubject}" << endl
				<< "make new attachment at vmessage with properties {file: vattachment}" << endl
				<< "open vmessage" << endl
				<< "end tell" << endl
				<< "end run" << endl;
		}
#if 0
		else if(CFStringCompare(appPath, CFSTR("/Applications/Thunderbird.app"), 0) == kCFCompareEqualTo)
		{
			// TODO: Handle Thunderbird here? Impossible?
		}
#endif
		CFRelease( appPath );
		p.closeWriteChannel();
		p.waitForFinished();
		if( p.exitCode() == 0 )
			return;
	}
#elif defined(Q_OS_LINUX)
	QByteArray thunderbird;
	QProcess p;
	QStringList env = QProcess::systemEnvironment();
	if( env.indexOf( QRegExp("KDE_FULL_SESSION.*") ) != -1 )
	{
		p.start( "kreadconfig", QStringList()
			<< "--file" << "emaildefaults"
			<< "--group" << "PROFILE_Default"
			<< "--key" << "EmailClient" );
		p.waitForFinished();
		QByteArray data = p.readAllStandardOutput().trimmed();
		if( data.contains("thunderbird") )
			thunderbird = data;
	}
	else if( env.indexOf( QRegExp("GNOME_DESKTOP_SESSION_ID.*") ) != -1 )
	{
		p.start( "gconftool-2", QStringList()
			<< "--get" << "/desktop/gnome/url-handlers/mailto/command" );
		p.waitForFinished();
		QByteArray data = p.readAllStandardOutput();
		if( data.contains("thunderbird") )
			thunderbird = data.split(' ').value(0);
	}
	/*
	else
	{
		p.start( "xprop", QStringList() << "-root" << "_DT_SAVE_MODE" );
		p.waitForFinished();
		if( p.readAllStandardOutput().contains("xfce4") )
		{}
	}*/

	bool status = false;
	if( !thunderbird.isEmpty() )
	{
		status = p.startDetached( thunderbird, QStringList() << "-compose"
			<< QString( "subject='%1',attachment='%2'" )
				.arg( url.queryItemValue( "subject" ) )
				.arg( QUrl::fromLocalFile( url.queryItemValue( "attachment" ) ).toString() ) );
	}
	else
	{
		status = p.startDetached( "xdg-email", QStringList()
			<< "--subject" << url.queryItemValue( "subject" )
			<< "--attach" << url.queryItemValue( "attachment" ) );
	}
	if( status )
		return;
#endif
	QDesktopServices::openUrl( url );
}

QString Common::normalized( const QString &str )
{
#if defined(Q_OS_MAC) && defined(QT_MAC_USE_COCOA)
	return str.normalized( QString::NormalizationForm_C );
#else
	return str;
#endif
}

QStringList Common::normalized( const QStringList &list )
{
#if defined(Q_OS_MAC) && defined(QT_MAC_USE_COCOA)
	QStringList l;
	Q_FOREACH( const QString &str, list )
		l << str.normalized( QString::NormalizationForm_C );
	return l;
#else
	return list;
#endif
}

bool Common::runPrivileged( const QString &program, const QStringList &arguments )
{
#ifdef USE_SECURITY
	QList<QByteArray> u8args;
	Q_FOREACH( const QString arg, arguments )
		u8args << arg.toUtf8();

	char *args[u8args.size() + 1];
	for( int i = 0; i < u8args.size(); ++i )
		args[i] = (char*)u8args[i].constData();
	args[u8args.size()] = 0;
	FILE *pipe = 0;

	AuthorizationRef ref;
	AuthorizationCreate( 0, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &ref );
	OSStatus status = AuthorizationExecuteWithPrivileges( ref, program.toUtf8().constData(),
		kAuthorizationFlagDefaults, args, &pipe );

	if( pipe )
		fclose( pipe );

	AuthorizationFree( ref, kAuthorizationFlagDestroyRights );
	return status == errAuthorizationSuccess;
#else
	Q_UNUSED( program )
	Q_UNUSED( arguments )
#endif
	return false;
}

void Common::showHelp( const QString &msg, int code )
{
	QUrl u;

	if( code > 0 )
	{
		u.setUrl( "http://www.sk.ee/digidoc/support/errorinfo/" );
		u.addQueryItem( "app", qApp->applicationName() );
		u.addQueryItem( "appver", qApp->applicationVersion() );
		u.addQueryItem( "l", Settings::language() );
		u.addQueryItem( "code", QString::number( code ) );
	}
	else
	{
		u.setUrl( helpUrl() );
		u.addQueryItem( "searchquery", msg );
		u.addQueryItem( "searchtype", "all" );
		u.addQueryItem( "_m", "core" );
		u.addQueryItem( "_a", "searchclient" );
	}
	QDesktopServices::openUrl( u );
}

bool Common::startDetached( const QString &program )
{ return startDetached( program, QStringList() ); }

bool Common::startDetached( const QString &program, const QStringList &arguments )
{
#ifdef Q_OS_MAC
	return QProcess::startDetached( "/usr/bin/open", QStringList() << "-a" << program << arguments );
#else
	return QProcess::startDetached( program, arguments );
#endif
}

QString Common::tempFilename()
{
	QString path = QDir::tempPath();
	QString prefix = QFileInfo( qApp->applicationFilePath() ).baseName();
#ifdef Q_OS_WIN
	wchar_t *name = _wtempnam( (wchar_t*)QDir::toNativeSeparators( path ).utf16(), (wchar_t*)prefix.utf16() );
	QString result = QString::fromUtf16( (ushort*)name );
#else
	char *name = tempnam( path.toLocal8Bit(), prefix.toLocal8Bit() );
	QString result = QString::fromLocal8Bit( name );
#endif
	free( name );
	return result;
}

#ifdef Q_OS_WIN
QByteArray Common::toLocal8Bit( const QString &str, bool utf8 )
{ return utf8 ? str.toUtf8() : str.toLocal8Bit(); }
#endif
