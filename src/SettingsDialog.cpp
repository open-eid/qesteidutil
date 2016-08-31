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
#include "ui_SettingsDialog.h"

#include "CertStore.h"
#include "QSmartCard.h"

#include <common/SslCertificate.h>

#include <QtCore/QProcess>
#include <QtWidgets/QPushButton>

#include <qt_windows.h>

static void runPrivileged( const QString &program, const QString &arguments )
{
	ShellExecuteW( 0, L"runas", PCWSTR(program.utf16()), PCWSTR(arguments.utf16()), 0, SW_HIDE );
}

SettingsDialog::SettingsDialog( const QSmartCardData &data, QWidget *parent )
	: QDialog( parent )
{
	Ui::SettingsDialog ui;
	ui.setupUi( this );
	setWindowModality( Qt::WindowModal );
	setAttribute( Qt::WA_DeleteOnClose, true );
	QPushButton *update = ui.buttonBox->addButton( tr("Check for updates and close utility"), QDialogButtonBox::ActionRole );
	QPushButton *sched = ui.buttonBox->addButton( tr("Run Task Scheduler"), QDialogButtonBox::ActionRole );
	QPushButton *clean = ui.buttonBox->addButton( tr("Clean certs"), QDialogButtonBox::ActionRole );

	int selected = QProcess::execute( "id-updater", QStringList() << "-status" );
	ui.updateInterval->setCurrentIndex( selected > 0 && selected < 4 ? selected : 2 );

	connect( ui.buttonBox, &QDialogButtonBox::clicked, [=](QAbstractButton *button ) {
		if( button == ui.buttonBox->button( QDialogButtonBox::Close ) )
			done( 0 );
		else if( button == update )
		{
			QProcess::startDetached( "id-updater" );
			done( 1 );
			qApp->exit();
		}
		else if( button == sched )
			runPrivileged( "control", "schedtasks" );
		else if( button == clean )
		{
			CertStore s;
			for( const SslCertificate &c: s.list())
			{
				if( c.subjectInfo( "O" ).contains("ESTEID") )
					s.remove( c );
			}
			QString personalCode = data.signCert().subjectInfo("serialNumber");

			if( !personalCode.isEmpty() ) {
				s.add( data.authCert(), data.card() );
				s.add( data.signCert(), data.card() );
			}
			done( 0 );
		}
	});
	connect( ui.updateInterval,
		static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [=]( int index )
	{
		switch( index )
		{
		case 1: return runPrivileged( "id-updater", "-daily" );
		case 2: return runPrivileged( "id-updater", "-weekly" );
		case 3: return runPrivileged( "id-updater", "-monthly" );
		case 4: return runPrivileged( "id-updater", "-remove" );
		default: break;
		}
	});
}
