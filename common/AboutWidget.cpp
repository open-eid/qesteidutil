#include "AboutWidget.h"

#include "ui_AboutWidget.h"

#include <QApplication>

class AboutWidgetPrivate: public Ui::AboutWidget
{};

AboutWidget::AboutWidget(QWidget *parent)
:	QWidget(parent)
,	d( new AboutWidgetPrivate )
{
	d->setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );
	setWindowFlags( Qt::Sheet );
	d->content->setText( QString("%1\n%2").arg( qApp->applicationName(), qApp->applicationVersion() ) );
}

AboutWidget::~AboutWidget() { delete d; }
