/*
 * QEstEidUtil
 *
 * Copyright (C) 2009 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009 Raul Metsma <raul@innovaatik.ee>
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
#include "common/Settings.h"

#include <QProcess>

SettingsDialog::SettingsDialog( QWidget *parent )
:	QDialog( parent )
{
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	Settings s;
	s.beginGroup( "Util" );

#ifdef WIN32
	updateInterval->addItem( tr("Once a day"), "-daily" );
	updateInterval->addItem( tr("Once a week"), "-weekly" );
	updateInterval->addItem( tr("Once a month"), "-monthly" );
	updateInterval->addItem( tr("Never"), "-remove" );
	int interval = updateInterval->findText( s.value( "updateInterval" ).toString() );
	if ( interval == -1 )
		interval = 0;
	updateInterval->setCurrentIndex( interval );
	autoUpdate->setChecked( s.value( "autoUpdate", true ).toBool() );
#else
	updateInterval->hide();
	updateIntervalLabel->hide();
	autoUpdate->hide();
	autoUpdateLabel->hide();
#endif
}

void SettingsDialog::accept()
{
	Settings s;
	s.beginGroup( "Util" );
	
	s.setValue( "updateInterval", updateInterval->currentText() );
	s.setValue( "autoUpdate", autoUpdate->isChecked() );

#ifdef WIN32
	QStringList list;
	if ( !autoUpdate->isChecked() )
		list << "-remove";
	else
		list << updateInterval->itemData( updateInterval->currentIndex() ).toString();
	QProcess::startDetached( "id-updater.exe", list );
#endif

	done( 1 );
}
