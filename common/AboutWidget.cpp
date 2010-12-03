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
#include <QDesktopServices>
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

	QString package;
#ifdef Q_OS_LINUX
	QProcess p;
	p.start( "dpkg-query", QStringList() << "-W" << "-f=${Package} ${Version}" << "estonianidcard" );
	if( !p.waitForStarted() && p.error() == QProcess::FailedToStart )
	{
		p.start( "rpm", QStringList() << "-q" << "--qf" << "%{NAME} %{VERSION}" << "estonianidcard" );
		p.waitForStarted();
	}
	p.waitForFinished();
	if( !p.exitCode() )
		package = "<br />" + QString::fromUtf8( p.readAll() );
	p.close();
#endif

	d->content->setText( trUtf8(
		"<center>%1 version %2, released mm.dd.yyyy%3<br /><br />"
		"Estonian ID-software is released by Riigi Infosüsteemide Arenduskeskus, developed by AS Sertifitseerimiskeskus<br /><br />"
		"Contact for assistance by email <a href=\"mailto:abi@id.ee\">abi@id.ee</a> or call 1777.</center>")
		.arg( qApp->applicationName(), qApp->applicationVersion(), package ) );
}

AboutWidget::~AboutWidget() { delete d; }

void AboutWidget::on_content_anchorClicked( const QUrl &link )
{ QDesktopServices::openUrl( link ); }
