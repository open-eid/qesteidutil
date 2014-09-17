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

#include "Common.h"

#include "SslCertificate.h"
#include "TokenData.h"
#include "Settings.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QPalette>
#include <QtGui/QTextDocument>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslError>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#else
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#endif

#include <stdlib.h>

#if defined(Q_OS_WIN)
#include <QtCore/QLibrary>

#include <qt_windows.h>
#include <mapi.h>
#include <Shellapi.h>
#elif defined(Q_OS_MAC)
#include <QtCore/QXmlStreamReader>
#include <sys/utsname.h>
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
static QString packageName( const QString &name, const QString &ver, bool withName )
{ return withName ? name + " (" + ver + ")" : ver; }
#endif

Common::Common( int &argc, char **argv )
	: BaseApplication( argc, argv )
{
	Q_INIT_RESOURCE(common_images);
	Q_INIT_RESOURCE(common_tr);
#if defined(Q_OS_WIN)
	setLibraryPaths( QStringList() << applicationDirPath() );
#elif defined(Q_OS_MAC)
	setLibraryPaths( QStringList() << applicationDirPath() + "/../PlugIns" );
#endif
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

	QNetworkProxyFactory::setUseSystemConfiguration(true);

#if defined(Q_OS_WIN)
	AllowSetForegroundWindow( ASFW_ANY );
#elif defined(Q_OS_MAC)
	macEvents = false;
#endif
}

Common::~Common()
{
#if defined(Q_OS_MAC)
	deinitMacEvents();
#endif
}

#ifndef Q_OS_MAC
void Common::addRecent( const QString & ) {}
#endif

QString Common::applicationOs()
{
#if defined(Q_OS_LINUX)
	QProcess p;
	p.start( "lsb_release", QStringList() << "-s" << "-d" );
	p.waitForFinished();
	return QString::fromLocal8Bit( p.readAll().trimmed() );
#elif defined(Q_OS_MAC)
	struct utsname unameData;
	uname( &unameData );
	QFile f( "/System/Library/CoreServices/SystemVersion.plist" );
	if( f.open( QFile::ReadOnly ) )
	{
		QXmlStreamReader xml( &f );
		while( xml.readNext() != QXmlStreamReader::Invalid )
		{
			if( !xml.isStartElement() || xml.name() != "key" || xml.readElementText() != "ProductVersion" )
				continue;
			xml.readNextStartElement();
			return QString( "Mac OS %1 (%2/%3)" )
				.arg( xml.readElementText() ).arg( QSysInfo::WordSize ).arg( unameData.machine );
		}
	}
#elif defined(Q_OS_WIN)
	OSVERSIONINFOEX osvi = { sizeof( OSVERSIONINFOEX ) };
	if( GetVersionEx( (OSVERSIONINFO *)&osvi ) )
	{
		bool workstation = osvi.wProductType == VER_NT_WORKSTATION;
		SYSTEM_INFO si;
		typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
		if( PGNSI pGNSI = PGNSI( QLibrary( "kernel32" ).resolve( "GetNativeSystemInfo" ) ) )
			pGNSI( &si );
		else
			GetSystemInfo( &si );
		QString os;
		switch( (osvi.dwMajorVersion << 8) + osvi.dwMinorVersion )
		{
		case 0x0500: os = workstation ? "2000 Professional" : "2000 Server"; break;
		case 0x0501: os = osvi.wSuiteMask & VER_SUITE_PERSONAL ? "XP Home" : "XP Professional"; break;
		case 0x0502:
			if( GetSystemMetrics( SM_SERVERR2 ) )
				os = "Server 2003 R2";
			else if( workstation && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
				os = "XP Professional";
			else
				os = "Server 2003";
			break;
		case 0x0600: os = workstation ? "Vista" : "Server 2008"; break;
		case 0x0601: os = workstation ? "7" : "Server 2008 R2"; break;
		case 0x0602: os = workstation ? "8" : "Server 2012"; break;
		case 0x0603: os = workstation ? "8.1" : "Server 2012 R2"; break;
		default: break;
		}
		QString extversion( (const QChar*)osvi.szCSDVersion );
		return QString( "Windows %1 %2(%3 bit)" ).arg( os )
			.arg( extversion.isEmpty() ? "" : extversion + " " )
			.arg( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "64" : "32" );
	}
	else
	{
		switch( QSysInfo::WindowsVersion )
		{
		case QSysInfo::WV_2000: return "Windows 2000";
		case QSysInfo::WV_XP: return "Windows XP";
		case QSysInfo::WV_2003: return "Windows 2003";
		case QSysInfo::WV_VISTA: return "Windows Vista";
		case QSysInfo::WV_WINDOWS7: return "Windows 7";
		case QSysInfo::WV_WINDOWS8: return "Windows 8";
		case QSysInfo::WV_WINDOWS8_1: return "Windows 8.1";
		default: break;
		}
	}
#endif

	return tr("Unknown OS");
}

quint8 Common::cardsOrderScore( QChar c )
{
	switch( c.toLatin1() )
	{
	case 'N': return 6;
	case 'A': return 5;
	case 'P': return 4;
	case 'E': return 3;
	case 'F': return 2;
	case 'B': return 1;
	default: return 0;
	}
}

bool Common::cardsOrder( const QString &s1, const QString &s2 )
{
	QRegExp r("(\\w{1,2})(\\d{7})");
	if( r.indexIn( s1 ) == -1 )
		return false;
	QStringList cap1 = r.capturedTexts();
	if( r.indexIn( s2 ) == -1 )
		return false;
	QStringList cap2 = r.capturedTexts();
	// new cards to front
	if( cap1[1].size() != cap2[1].size() )
		return cap1[1].size() > cap2[1].size();
	// card type order
	if( cap1[1][0] != cap2[1][0] )
		return cardsOrderScore( cap1[1][0] ) > cardsOrderScore( cap2[1][0] );
	// card version order
	if( cap1[1].size() > 1 && cap2[1].size() > 1 && cap1[1][1] != cap2[1][1] )
		return cap1[1][1] > cap2[1][1];
	// serial number order
	return cap1[2].toUInt() > cap2[2].toUInt();
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
	if( QProcess::startDetached( "open", QStringList() << "-R" << u.toLocalFile() ) )
		return;
#endif
	QDesktopServices::openUrl( QUrl::fromLocalFile( QFileInfo( u.toLocalFile() ).absolutePath() ) );
}

void Common::detectPlugins()
{
#if defined(Q_OS_MAC) && !defined(INTERNATIONAL)
	if( QSettings().value("plugins").isNull() )
		QTimer::singleShot( 1000, this, SLOT(showPlugins()) );
#endif
}

QUrl Common::helpUrl()
{
	QString lang = Settings::language();
	QUrl u( "http://support.sk.ee" );
	if( lang == "en" ) u = "http://support.sk.ee/eng/";
	if( lang == "ru" ) u = "http://support.sk.ee/ru/";
	u.addQueryItem( "app", applicationName() );
	u.addQueryItem( "appver", applicationVersion() );
	u.addQueryItem( "os", applicationOs() );
	return u;
}

bool Common::event( QEvent *e )
{
#ifdef Q_OS_MAC
	// Load here because cocoa NSApplication overides events
	if( e->type() == QEvent::ApplicationActivate )
		initMacEvents();
#endif
	return BaseApplication::event( e );
}

#ifndef Q_OS_MAC
void Common::mailTo( const QUrl &url )
{
#if defined(Q_OS_WIN)
	QString file = url.queryItemValue( "attachment" );
	QLibrary lib("mapi32");
	if( LPMAPISENDMAILW mapi = LPMAPISENDMAILW(lib.resolve("MAPISendMailW")) )
	{
		MapiFileDescW doc = { 0, 0, 0, 0, 0, 0 };
		doc.nPosition = -1;
		doc.lpszPathName = (PWSTR)QDir::toNativeSeparators( file ).unicode();
		doc.lpszFileName = (PWSTR)QFileInfo( file ).fileName().unicode();
		MapiMessageW message = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		message.lpszSubject =  (PWSTR)url.queryItemValue( "subject" ).unicode();
		message.lpszNoteText = L"";
		message.nFileCount = 1;
		message.lpFiles = lpMapiFileDescW(&doc);
		switch( mapi( NULL, 0, &message, MAPI_LOGON_UI|MAPI_DIALOG, 0 ) )
		{
		case SUCCESS_SUCCESS:
		case MAPI_E_USER_ABORT:
		case MAPI_E_LOGIN_FAILURE:
			return;
		default: break;
		}
	}
	else if( LPMAPISENDMAIL mapi = LPMAPISENDMAIL(lib.resolve("MAPISendMail")) )
	{
		QByteArray filePath = QDir::toNativeSeparators( file ).toLocal8Bit();
		QByteArray fileName = QFileInfo( file ).fileName().toLocal8Bit();
		QByteArray subject = url.queryItemValue( "subject" ).toLocal8Bit();
		MapiFileDesc doc = { 0, 0, 0, 0, 0, 0 };
		doc.nPosition = -1;
		doc.lpszPathName = const_cast<char*>(filePath.constData());
		doc.lpszFileName = const_cast<char*>(fileName.constData());
		MapiMessage message = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		message.lpszSubject = const_cast<char*>(subject.constData());
		message.lpszNoteText = "";
		message.nFileCount = 1;
		message.lpFiles = lpMapiFileDesc(&doc);
		switch( mapi( NULL, 0, &message, MAPI_LOGON_UI|MAPI_DIALOG, 0 ) )
		{
		case SUCCESS_SUCCESS:
		case MAPI_E_USER_ABORT:
		case MAPI_E_LOGIN_FAILURE:
			return;
		default: break;
		}
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
		if(QSettings(QDir::homePath() + "/.local/share/applications/mimeapps.list", QSettings::IniFormat)
				.value("Default Applications/x-scheme-handler/mailto").toString().contains("thunderbird"))
			thunderbird = "/usr/bin/thunderbird";
		else
		{
			for(const QString &path: QProcessEnvironment::systemEnvironment().value("XDG_DATA_DIRS").split(":"))
			{
				if(QSettings(path + "/applications/defaults.list", QSettings::IniFormat)
						.value("Default Applications/x-scheme-handler/mailto").toString().contains("thunderbird"))
				{
					thunderbird = "/usr/bin/thunderbird";
					break;
				}
			}
		}
	}

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
#endif

#ifndef Q_OS_MAC
QStringList Common::packages( const QStringList &names, bool withName )
{
	QStringList packages;
#if defined(Q_OS_WIN)
	QString path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
#if 1
	int count = applicationOs().contains( "64" ) ? 4 : 2;
	for( int i = 0; i < count; ++i )
	{
		HKEY reg = i % 2 == 0 ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
		REGSAM param = KEY_READ|(i >= 2 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);
		HKEY key;
		long result = RegOpenKeyEx( reg, LPCWSTR(path.utf16()), 0, param, &key );
		if( result != ERROR_SUCCESS )
			continue;

		DWORD numSubgroups = 0, maxSubgroupSize = 0;
		result = RegQueryInfoKey( key, 0, 0, 0, &numSubgroups, &maxSubgroupSize, 0, 0, 0, 0, 0, 0 );
		if( result != ERROR_SUCCESS )
		{
			RegCloseKey( key );
			continue;
		}

		for( DWORD j = 0; j < numSubgroups; ++j )
		{
			DWORD groupSize = maxSubgroupSize + 1;
			QString group( groupSize, 0 );
			result = RegEnumKeyEx( key, j, LPWSTR(group.data()), &groupSize, 0, 0, 0, 0 );
			if( result != ERROR_SUCCESS )
				continue;
			group.resize( groupSize );

			HKEY subkey;
			QString subpath = path + "\\" + group;
			result = RegOpenKeyEx( reg, LPCWSTR(subpath.utf16()), 0, param, &subkey );
			if( result != ERROR_SUCCESS )
				continue;

			DWORD numKeys = 0, maxKeySize = 0, maxValueSize = 0;
			result = RegQueryInfoKey( subkey, 0, 0, 0, 0, 0, 0, &numKeys, &maxKeySize, &maxValueSize, 0, 0 );
			if( result != ERROR_SUCCESS )
			{
				RegCloseKey( subkey );
				continue;
			}

			QString name;
			QString version;
			QString type;
			for( DWORD k = 0; k < numKeys; ++k )
			{
				DWORD dataType = 0;
				DWORD keySize = maxKeySize + 1;
				DWORD dataSize = maxValueSize;
				QString key( keySize, 0 );
				QByteArray data( dataSize, 0 );

				result = RegEnumValue( subkey, k, LPWSTR(key.data()), &keySize, 0,
					&dataType, (unsigned char*)data.data(), &dataSize );
				if( result != ERROR_SUCCESS )
					continue;
				key.resize( keySize );
				data.resize( dataSize );

				QString value;
				switch( dataType )
				{
				case REG_SZ:
					value = QString::fromUtf16( (const ushort*)data.constData() );
					break;
				default: continue;
				}

				if( key == "DisplayName" ) name = value;
				if( key == "DisplayVersion" ) version = value;
				if( key == "ReleaseType" ) type = value;
			}
			RegCloseKey( subkey );

			if( !type.contains( "Update", Qt::CaseInsensitive ) &&
				name.contains( QRegExp( names.join( "|" ), Qt::CaseInsensitive ) ) )
				packages << packageName( name, version, withName );
		}
		RegCloseKey( key );
	}
	packages.removeDuplicates();
#else // problems on 64bit windows
	Q_FOREACH( const QString &group, QStringList() << "HKEY_LOCAL_MACHINE" << "HKEY_CURRENT_USER" )
	{
		QSettings s( group + "\\" + path, QSettings::NativeFormat );
		Q_FOREACH( const QString &key, s.childGroups() )
		{
			QString name = s.value( key + "/DisplayName" ).toString();
			QString version = s.value( key + "/DisplayVersion" ).toString();
			QString type = s.value( key + "/ReleaseType" ).toString();
			if( !type.contains( "Update", Qt::CaseInsensitive ) &&
				name.contains( QRegExp( names.join( "|" ), Qt::CaseInsensitive ) ) )
				packages << packageName( name, version, withName );
		}
	}
#endif
#elif defined(Q_OS_LINUX)
	QProcess p;

	Q_FOREACH( const QString &name, names )
	{
		p.start( "dpkg-query", QStringList() << "-W" << "-f=${Version}" << name );
		if( !p.waitForStarted() && p.error() == QProcess::FailedToStart )
		{
			p.start( "rpm", QStringList() << "-q" << "--qf" << "%{VERSION}" << name );
			p.waitForStarted();
		}
		p.waitForFinished();
		if( !p.exitCode() )
		{
			QString ver = QString::fromLocal8Bit( p.readAll().trimmed() );
			if( !ver.isEmpty() )
				packages << packageName( name, ver, withName );
		}
	}
#endif
	return packages;
}
#endif

void Common::setAccessibleName( QLabel *widget )
{
	QTextDocument doc;
	doc.setHtml( widget->text() );
	widget->setAccessibleName( doc.toPlainText() );
}

void Common::showHelp( const QString &msg, int code )
{
	QUrl u;

	if( code > 0 )
	{
		u.setUrl( "http://www.sk.ee/digidoc/support/errorinfo/" );
		u.addQueryItem( "app", applicationName() );
		u.addQueryItem( "appver", applicationVersion() );
		u.addQueryItem( "l", Settings::language() );
		u.addQueryItem( "code", QString::number( code ) );
		u.addQueryItem( "os", applicationOs() );
	}
	else
	{
		u = helpUrl();
		u.addQueryItem( "searchquery", msg );
		u.addQueryItem( "searchtype", "all" );
		u.addQueryItem( "_m", "core" );
		u.addQueryItem( "_a", "searchclient" );
	}
	QDesktopServices::openUrl( u );
}

void Common::showPlugins()
{
#ifdef Q_OS_MAC
	QMessageBox *b = new QMessageBox( QMessageBox::Information, tr("Browser plugins"),
		tr("If you are using e-services for authentication and signing documents in addition to "
			"Mobile-ID an ID-card or only ID-card, you should install the browser integration packages.<br />"
			"<a href='http://installer.id.ee'>http://installer.id.ee</a>" ),
		0, activeWindow() );
	QAbstractButton *install = b->addButton( tr("Install"), QMessageBox::AcceptRole );
	b->addButton( tr("Remind later"), QMessageBox::AcceptRole );
	QAbstractButton *ignore = b->addButton( tr("Ignore forever"), QMessageBox::AcceptRole );
	b->exec();
	if( b->clickedButton() == install )
		QDesktopServices::openUrl( QUrl("http://installer.id.ee") );
	else if( b->clickedButton() == ignore )
		QSettings().setValue( "plugins", "ignore" );
#endif
}

void Common::validate()
{
#ifdef KILLSWITCH
	Settings s(qApp->applicationName());
	if(s.value("LastCheck").isNull())
		s.setValue("LastCheck", QDate::currentDate().toString("yyyyMMdd"));
	QDate lastCheck = QDate::fromString(s.value("LastCheck").toString(), "yyyyMMdd");
	if(lastCheck > QDate::currentDate().addDays(-7))
		return;
	QNetworkRequest req(QUrl(KILLSWITCH));
	req.setRawHeader("Accept-Language", Settings().language().toUtf8());
	req.setRawHeader("User-Agent", QString("%1/%2 (%3)")
		.arg(applicationName(), applicationVersion(), applicationOs()).toUtf8());
	QNetworkReply *reply = (new QNetworkAccessManager())->get(req);
	connect(reply, &QNetworkReply::sslErrors, [=](const QList<QSslError> &errors){
		QList<QSslError> ignore;
		for(const QSslError &e: errors)
		{
			switch(e.error()){
			case QSslError::SelfSignedCertificateInChain:
			case QSslError::HostNameMismatch:
				ignore << e;
				break;
			default: qWarning() << e << e.certificate().subjectInfo("CN");
			}
		}
		reply->ignoreSslErrors(ignore);
	});
	connect(reply, &QNetworkReply::finished, [=](){
		Settings s(qApp->applicationName());
		switch(reply->error())
		{
		case QNetworkReply::NoError:
		{
			if(reply->sslConfiguration().peerCertificate().subjectInfo("CN").value(0) == "installer.id.ee" &&
				reply->header( QNetworkRequest::ContentTypeHeader ) == "application/x-killswitch")
			{
				QString message = QString::fromUtf8(reply->readAll());
				if(!message.isEmpty())
				{
					QMessageBox::critical(activeWindow(), QString(), message);
					qApp->quit();
				}
				else
					s.setValue("LastCheck", QDate::currentDate().toString("yyyyMMdd"));
				break;
			}
		}
		default:
			if(lastCheck < QDate::currentDate().addMonths(-12))
			{
				QMessageBox::critical(activeWindow(), QString(), tr(
					"The software support period verification failed. You're not allowed to use this program. Please check your internet connection. "
					"<a href=\"http://www.id.ee/index.php?id=36738\">Additional info</a>."));
				qApp->quit();
			}
			else
			{
				if(s.value("LastCheckWarning", false).toBool())
					break;
				QMessageBox box(QMessageBox::Warning, QString(), tr(
					"The software support period verification failed. Please check your internet connection. "
					"<a href=\"http://www.id.ee/index.php?id=36738\">Additional info</a>."), 0, activeWindow());
				QPushButton *button = box.addButton(tr("Don't show this message again."), QMessageBox::AcceptRole);
				box.addButton(QMessageBox::Ok);
				box.exec();
				s.setValue("LastCheckWarning", box.clickedButton() == button);
			}
		}
		reply->manager()->deleteLater();
	});
#endif
}
