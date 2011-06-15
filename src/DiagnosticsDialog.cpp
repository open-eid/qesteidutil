/*
 * QEstEidUtil
 *
 * Copyright (C) 2009-2011 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009-2011 Raul Metsma <raul@innovaatik.ee>
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
#include "DiagnosticsDialog.h"

#include <smartcardpp/common.h>
#include <smartcardpp/PCSCManager.h>
#include <smartcardpp/esteid/EstEidCard.h>

#include <common/SslCertificate.h>

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSslCertificate>
#include <QTextStream>

QString DiagnosticsDialog::getReaderInfo()
{
	QString d;
	QTextStream s( &d );

	QHash<QString,QString> readers;
	QString reader;
	PCSCManager *m = 0;
	try {
		m = new PCSCManager();
	} catch( const std::runtime_error & ) {
		readers[reader] = tr("No readers found");
	}
	if ( m )
	{
		try {
			int readersCount = m->getReaderCount( true );
			for( int i = 0; i < readersCount; i++ )
			{
				reader = QString::fromStdString( m->getReaderName( i ) );
				if ( m->isPinPad( i ) )
					reader.append( " - PinPad" );
				if ( !QString::fromStdString( m->getReaderState( i ) ).contains( "EMPTY" ) )
				{
					EstEidCard card( *m, i );
					QString id = QString::fromStdString( card.readCardID() );
					ByteVec certBytes = card.getAuthCert();
					if( certBytes.size() )
					{
						QSslCertificate cert = QSslCertificate( QByteArray((char *)&certBytes[0], (int)certBytes.size()), QSsl::Der );
						if ( cert.isValid() && SslCertificate( cert ).isDigiID() )
							id = cert.subjectInfo( "serialNumber" );
					}
					readers[reader] = QString( "ID - %1<br />ATR - %2" )
						.arg( id )
						.arg( QString::fromStdString( m->getATRHex( i ) ).toUpper() );
				} else
					readers[reader] = "";
			}
		} catch( const std::runtime_error &e ) {
			readers[reader] = tr("Error reading card data:") + e.what();
		}
	}
	if ( m )
		delete m;

	for( QHash<QString,QString>::const_iterator i = readers.constBegin();
		i != readers.constEnd(); ++i )
	{
		if ( !i.key().isEmpty() )
			s << "* " << i.key();
		if( !i.value().isEmpty() )
			s << "<p style='margin-left: 10px; margin-top: 0px; margin-bottom: 0px; margin-right: 0px;'>" << i.value() << "</p>";
		else
			s << "<br />";
	}

	return d;
}

void DiagnosticsDialog::save()
{
	QString filename = QFileDialog::getSaveFileName( this, tr("Save as"), QString( "%1%2qesteidutil_diagnostics.txt" )
		.arg( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) ).arg( QDir::separator() ),
		tr("Text files (*.txt)") );
	if( filename.isEmpty() )
		return;
	QFile f( filename );
	if( f.open( QIODevice::WriteOnly ) )
		f.write( diagnosticsText->toPlainText().toUtf8() );
	else
		QMessageBox::warning( this, tr("Error occurred"), tr("Failed write to file!") );
}
