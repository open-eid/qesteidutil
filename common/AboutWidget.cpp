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

#include "AboutWidget.h"

#include "ui_AboutWidget.h"

#include <QApplication>
#include <QProcess>

class AboutWidgetPrivate: public Ui::AboutWidget
{};

AboutWidget::AboutWidget(QWidget *parent)
:	QWidget(parent)
,	d( new AboutWidgetPrivate )
{
	d->setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );
	setWindowFlags( Qt::Sheet );

	QProcess p;
	p.start( "dpkg-query", QStringList() << "-W" << "-f=${Package} ${Version}" << "estonianidcard" );
	if( !p.waitForStarted() && p.error() == QProcess::FailedToStart )
	{
		p.start( "rpm", QStringList() << "-q" << "--qf" << "'%{VERSION}'" << "estonianidcard" );
		p.waitForStarted();
	}
	p.waitForFinished();
	QString package;
	if( !p.exitCode() )
		package = QString::fromUtf8( p.readAll() );
	p.close();

	d->content->setText( QString("<center>%1<br />%2<br />%3</center>")
		.arg( qApp->applicationName(), qApp->applicationVersion(), package ) );
}

AboutWidget::~AboutWidget() { delete d; }
