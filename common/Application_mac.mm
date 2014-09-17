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

#include <Cocoa/Cocoa.h>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

@implementation NSApplication (ApplicationObjC)

- (void)appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	Q_UNUSED(event)
	Q_UNUSED(replyEvent)
	QApplication::postEvent( qApp, new REOpenEvent );
}

- (void)openClient:(NSPasteboard *)pboard userData:(NSString *)data error:(NSString **)error
{
	Q_UNUSED(data)
	Q_UNUSED(error)
	QStringList result;
	for( NSString *filename in [pboard propertyListForType:NSFilenamesPboardType] )
		result << QString::fromNSString( filename );
	QMetaObject::invokeMethod( qApp, "showClient", Q_ARG(QStringList,result) );
}

- (void)openCrypto:(NSPasteboard *)pboard userData:(NSString *)data error:(NSString **)error
{
	Q_UNUSED(data)
	Q_UNUSED(error)
	QStringList result;
	for( NSString *filename in [pboard propertyListForType:NSFilenamesPboardType] )
		result << QString::fromNSString( filename );
	QMetaObject::invokeMethod( qApp, "showCrypto", Q_ARG(QStringList,result) );
}
@end

void Common::addRecent( const QString &file )
{
	if( !file.isEmpty() )
		[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:file.toNSString()]];
}

void Common::initMacEvents()
{
	if( macEvents )
		return;
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:NSApp
		andSelector:@selector(appReopen:withReplyEvent:)
		forEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
	// reload /System/Library/CoreServices/pbs
	// list /System/Library/CoreServices/pbs -dump_pboard
	[NSApp setServicesProvider:NSApp];
	macEvents = true;
}

void Common::deinitMacEvents()
{
	[[NSAppleEventManager sharedAppleEventManager]
		removeEventHandlerForEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
}

void Common::mailTo( const QUrl &url )
{
	CFURLRef appUrl = 0;
	if( LSGetApplicationForURL( (__bridge CFURLRef)url.toNSURL(), kLSRolesAll, NULL, &appUrl ) == noErr )
	{
		NSString *appPath = [((__bridge NSURL *)appUrl) path];
		CFRelease( appUrl );
		QString p;
		QTextStream s( &p );
		if( [appPath rangeOfString:@"/Applications/Mail.app"].location != NSNotFound )
		{
			s << "on run" << endl
			<< "set vattachment to \"" << url.queryItemValue("attachment") << "\"" << endl
			<< "set vsubject to \"" << url.queryItemValue("subject") << "\"" << endl
			<< "tell application \"Mail\"" << endl
			<< "set msg to make new outgoing message with properties {subject:vsubject, visible:true}" << endl
			<< "tell content of msg to make new attachment with properties {file name:(vattachment as POSIX file) as alias}" << endl
			<< "activate" << endl
			<< "end tell" << endl
			<< "end run" << endl;
		}
		else if( [appPath rangeOfString:@"Entourage"].location != NSNotFound )
		{
			s << "on run" << endl
			<< "set vattachment to \"" << url.queryItemValue("attachment") << "\"" << endl
			<< "set vsubject to \"" << url.queryItemValue("subject") << "\"" << endl
			<< "tell application \"Microsoft Entourage\"" << endl
			<< "set vmessage to make new outgoing message with properties" << endl
			<< "{subject:vsubject, attachments:vattachment}" << endl
			<< "open vmessage" << endl
			<< "activate" << endl
			<< "end tell" << endl
			<< "end run" << endl;
		}
		else if( [appPath rangeOfString:@"Outlook"].location != NSNotFound )
		{
			s << "on run" << endl
			<< "set vattachment to \"" << url.queryItemValue("attachment") << "\"" << endl
			<< "set vsubject to \"" << url.queryItemValue("subject") << "\"" << endl
			<< "tell application \"Microsoft Outlook\"" << endl
			<< "activate" << endl
			<< "set vmessage to make new outgoing message with properties {subject:vsubject}" << endl
			<< "make new attachment at vmessage with properties {file: vattachment}" << endl
			<< "open vmessage" << endl
			<< "end tell" << endl
			<< "end run" << endl;
		}
#if 0
		else if([appPath rangeOfString:"/Applications/Thunderbird.app"].location != NSNotFound)
		{
			// TODO: Handle Thunderbird here? Impossible?
		}
#endif
		if(!p.isEmpty())
		{
			NSAppleScript *appleScript = [[NSAppleScript alloc] initWithSource:p.toNSString()];
			NSDictionary *err;
			if([appleScript executeAndReturnError:&err])
				return;
		}
	}
	QDesktopServices::openUrl( url );
}

QStringList Common::packages(const QStringList &names, bool)
{
	QStringList result;
	for (const QString &name: names) {
		NSBundle *bundle = [NSBundle bundleWithIdentifier:QString("ee.ria." + name).toNSString()];
		if (!bundle)
			continue;
		result << QString("%1 (%2.%3)").arg(name)
			.arg(QString::fromNSString(bundle.infoDictionary[@"CFBundleShortVersionString"]))
			.arg(QString::fromNSString(bundle.infoDictionary[@"CFBundleVersion"]));
	}
	return result;
}
