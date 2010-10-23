/*
 * QEstEidCommon
 *
 * Copyright (C) 2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010 Raul Metsma <raul@innovaatik.ee>
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

#include "Application_mac.h"
#include "Common.h"

#include <QApplication>

#ifdef QT_MAC_USE_COCOA
#include <Cocoa/Cocoa.h>

@interface ApplicationObjC : NSObject {
}

- (id) init;
- (void) appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
@end

@implementation ApplicationObjC
- (id) init
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[super init];
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
		andSelector:@selector(appReopen:withReplyEvent:)
		forEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
	[pool release];
	return self;
}

- (void) dealloc
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
	[pool release];
	[super dealloc];
}

- (void) appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	QApplication::postEvent( qApp, new REOpenEvent );
}
@end

struct ApplicationStruct { ApplicationObjC *applicationObjC; };

ApplicationObj::ApplicationObj( QObject *parent )
: QObject( parent )
, eventsLoaded( false )
, d( new ApplicationStruct )
{
	d->applicationObjC = [[ApplicationObjC alloc] init];
	qApp->installEventFilter( this );
}

ApplicationObj::~ApplicationObj()
{
	[d->applicationObjC release];
	delete d;
}

bool ApplicationObj::eventFilter( QObject *o, QEvent *e )
{
	// Load here because cocoa NSApplication overides events
	if( o == qApp && e->type() == QEvent::ApplicationActivate && !eventsLoaded )
	{
		mac_install_event_handler( this );
		eventsLoaded = true;
	}
	return QObject::eventFilter( o, e );
}

#else
#include <Carbon/Carbon.h>
static OSStatus appleEventHandler( const AppleEvent *event, AppleEvent *, long )
{
	QApplication::postEvent( qApp, new REOpenEvent );
	return 0;
}
#endif

void mac_install_event_handler( QObject *app )
{
#ifdef QT_MAC_USE_COCOA
	new ApplicationObj( app );
#else
	Q_UNUSED( app )
	AEEventHandlerUPP appleEventHandlerPP = AEEventHandlerUPP(appleEventHandler);
	AEInstallEventHandler( kCoreEventClass, kAEReopenApplication, appleEventHandlerPP, 0, false );
#endif
}
