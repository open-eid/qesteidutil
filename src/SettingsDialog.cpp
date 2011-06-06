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

#include <common/Common.h>

SettingsDialog::SettingsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );
	updateInterval->addItem( tr("Once a day"), "-daily" );
	updateInterval->addItem( tr("Once a week"), "-weekly" );
	updateInterval->addItem( tr("Once a month"), "-monthly" );
	updateInterval->addItem( tr("Never"), "-never" );
	updateInterval->addItem( tr("Remove"), "-remove" );
}

void SettingsDialog::accept()
{
#ifdef Q_OS_MAC
	Common::runPrivileged( "/Library/EstonianIDCard/bin/id-updater.app/Contents/MacOS/id-updater",
#else
	Common::startDetached( "id-updater",
#endif
		QStringList()
			<< updateInterval->itemData( updateInterval->currentIndex() ).toString()
			<< (autoUpdate->isChecked() ? "-autoupdate" : "") );
	done( 1 );
}

void SettingsDialog::on_checkUpdates_clicked()
{
	Common::startDetached( "id-updater" );
	done( 1 );
}
