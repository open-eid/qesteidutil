#include <QApplication>
#include "Application_mac.h"

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
	if( self = [super init] )
	{
		[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
			andSelector:@selector(appReopen:withReplyEvent:)
			forEventClass:kCoreEventClass
			andEventID:kAEReopenApplication];
	}
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
, d( new ApplicationStruct )
{
	d->applicationObjC = [[ApplicationObjC alloc] init];
}

ApplicationObj::~ApplicationObj()
{
	[d->applicationObjC release];
	delete d;
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
	AEEventHandlerUPP appleEventHandlerPP = AEEventHandlerUPP(appleEventHandler);
	AEInstallEventHandler( kCoreEventClass, kAEReopenApplication, appleEventHandlerPP, 0, false );
#endif
}
