/*
 * QEstEidCommon
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "Common.h"
#include "Diagnostics.h"

#include <QtCore/QFile>
#include <QtCore/QThreadPool>
#include <QtGui/QDesktopServices>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#endif

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QString package;
#ifndef Q_OS_MAC
	QStringList packages = Common::packages( QStringList() << "Eesti ID-kaardi tarkvara" << "estonianidcard" );
	if( !packages.isEmpty() )
		package = "<br />" + tr("Base version:") + " " + packages.first();
#endif
	ui->version->setText( tr("%1 version %2, released %3%4")
		.arg( qApp->applicationName(), qApp->applicationVersion(), BUILD_DATE, package ) );

	ui->diagnosticsTab->setEnabled( false );
	QApplication::setOverrideCursor( Qt::WaitCursor );
	Diagnostics *worker = new Diagnostics();
	connect( worker, SIGNAL(update(QString)), ui->diagnostics, SLOT(insertHtml(QString)), Qt::QueuedConnection );
	connect( worker, SIGNAL(destroyed()), SLOT(restoreCursor()) );
	QThreadPool::globalInstance()->start( worker );
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::openTab( int index )
{
	ui->tabWidget->setCurrentIndex( index );
	open();
}

void AboutDialog::restoreCursor()
{
	ui->diagnosticsTab->setEnabled( true );
	QApplication::restoreOverrideCursor();
}

void AboutDialog::saveDiagnostics()
{
	QString filename = QFileDialog::getSaveFileName( this, tr("Save as"), QString( "%1/%2_diagnostics.txt")
		.arg( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ), qApp->applicationName() ),
		tr("Text files (*.txt)") );
	if( filename.isEmpty() )
		return;
	QFile f( filename );
	if( !f.open( QIODevice::WriteOnly ) || !f.write( ui->diagnostics->toPlainText().toUtf8() ) )
		QMessageBox::warning( this, tr("Error occurred"), tr("Failed write to file!") );
}
