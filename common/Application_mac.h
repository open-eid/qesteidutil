#pragma once

#include <QEvent>

#ifdef QT_MAC_USE_COCOA
#include <QObject>
struct ApplicationStruct;
class ApplicationObj: public QObject
{
	Q_OBJECT
public:
	ApplicationObj( QObject *parent = 0 );
	~ApplicationObj();
private:
	ApplicationStruct *d;
};
#endif

class REOpenEvent: public QEvent
{
public:
	enum { Type = QEvent::User + 1 };
	REOpenEvent(): QEvent( QEvent::Type(Type) ) {}
};
