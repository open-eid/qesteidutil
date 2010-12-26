/*
 * QEstEidUtil
 *
 * Copyright (C) 2009,2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009,2010 Raul Metsma <raul@innovaatik.ee>
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

#include "SettingsDialog.h"

#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

SettingsDialog::SettingsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	updateInterval->addItem( tr("Once a day"), "-daily" );
	updateInterval->addItem( tr("Once a week"), "-weekly" );
	updateInterval->addItem( tr("Once a month"), "-monthly" );
	updateInterval->addItem( tr("Disabled"), "-never" );
	updateInterval->addItem( tr("Remove"), "-remove" );

#ifdef Q_OS_MAC
	updateInterval->hide();
	updateIntervalLabel->hide();
	autoUpdate->hide();
	autoUpdateLabel->hide();
	buttonBox->setStandardButtons( QDialogButtonBox::Close );
#endif
}

void SettingsDialog::accept()
{
	QProcess::startDetached( "id-updater.exe", QStringList() <<
		updateInterval->itemData( updateInterval->currentIndex() ).toString() );
	done( 1 );
}

void SettingsDialog::on_checkUpdates_clicked()
{
#ifdef Q_OS_WIN32
	QProcess::startDetached( "id-updater.exe" );
#else
	QDesktopServices::openUrl( QUrl(
		QString("https://installer.id.ee/update/mac/?ver=%1")
			  .arg( QCoreApplication::applicationVersion() ) ) );
#endif
	done( 1 );
}
