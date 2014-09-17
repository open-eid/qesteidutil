/*
 * QEstEidUtil
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

#include <QtCore/QProcess>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include <qt_windows.h>

SettingsDialog::SettingsDialog( QWidget *parent )
:	QDialog( parent )
,	update(0)
,	sched(0)
{
	setupUi( this );
	setWindowModality( Qt::WindowModal );
	setAttribute( Qt::WA_DeleteOnClose, true );
	updateInterval->addItem( "" );
	updateInterval->addItem( tr("Once a day"), "-daily" );
	updateInterval->addItem( tr("Once a week"), "-weekly" );
	updateInterval->addItem( tr("Once a month"), "-monthly" );
	updateInterval->addItem( tr("Remove"), "-remove" );
	update = buttonBox->addButton( tr("Check for updates and close utility"), QDialogButtonBox::ActionRole );
	sched = buttonBox->addButton( tr("Run Task Scheduler"), QDialogButtonBox::ActionRole );
	int selected = QProcess::execute( "id-updater", QStringList() << "-status" );
	updateInterval->setCurrentIndex( selected > 0 && selected < 4 ? selected : 2 );
}

void SettingsDialog::buttonClicked( QAbstractButton *button )
{
	if( button == buttonBox->button( QDialogButtonBox::Close ) )
		done( 0 );
	else if( button == update )
	{
		QProcess::startDetached( "id-updater" );
		done( 1 );
		qApp->exit();
	}
	else if( button == sched )
		runPrivileged( "control", QStringList() << "schedtasks" );
}

void SettingsDialog::on_updateInterval_activated( int index )
{
	if( index == 0 )
		return;
	runPrivileged( "id-updater",
		QStringList() << updateInterval->itemData( index ).toString() );
}

bool SettingsDialog::runPrivileged( const QString &program, const QStringList &arguments )
{
	QString params = arguments.join(" ");
	HINSTANCE result = ShellExecuteW( 0, L"runas", PCWSTR(program.utf16()), PCWSTR(params.utf16()), 0, SW_HIDE );
	return int(result) > 32;
	return QProcess::startDetached( program, arguments );
}
