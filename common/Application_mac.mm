/*
 * QEstEidCommon
 *
 * Copyright (C) 2010-2011 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010-2011 Raul Metsma <raul@innovaatik.ee>
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
	Q_UNUSED(event)
	Q_UNUSED(replyEvent)
	QApplication::postEvent( qApp, new REOpenEvent );
}
@end

struct ApplicationStruct { ApplicationObjC *applicationObjC; };

#else
#include <Carbon/Carbon.h>
static OSStatus appleEventHandler( const AppleEvent *, AppleEvent *, long )
{
	QApplication::postEvent( qApp, new REOpenEvent );
	return 0;
}
#endif

void Common::initMacEvents()
{
#ifdef QT_MAC_USE_COCOA
	if( macEvents )
		return;
	macEvents = new ApplicationStruct;
	macEvents->applicationObjC = [[ApplicationObjC alloc] init];
#else
	AEEventHandlerUPP appleEventHandlerPP = AEEventHandlerUPP(appleEventHandler);
	AEInstallEventHandler( kCoreEventClass, kAEReopenApplication, appleEventHandlerPP, 0, false );
#endif
}

void Common::deinitMacEvents()
{
#ifdef QT_MAC_USE_COCOA
	if( !macEvents )
		return;
	[macEvents->applicationObjC release];
	delete macEvents;
#endif
}
