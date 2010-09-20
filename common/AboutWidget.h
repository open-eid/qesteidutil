#pragma once

#include <QWidget>

class AboutWidgetPrivate;
class AboutWidget: public QWidget
{
    Q_OBJECT
public:
	explicit AboutWidget( QWidget *parent = 0 );
	~AboutWidget();

private:
	AboutWidgetPrivate *d;
};
