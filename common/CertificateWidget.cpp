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

#include "CertificateWidget.h"

#include "ui_CertificateWidget.h"
#include "DateTime.h"
#include "SslCertificate.h"

#include <QtCore/QTextStream>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QSslKey>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#endif

class CertificateDialogPrivate: public Ui::CertificateDialog
{
public:
	void addItem( const QString &variable, const QString &value, const QVariant &valueext = QVariant() );

	SslCertificate cert;
};

void CertificateDialogPrivate::addItem( const QString &variable, const QString &value, const QVariant &valueext )
{
	QTreeWidgetItem *t = new QTreeWidgetItem( parameters );
	t->setText( 0, variable );
	t->setText( 1, value );
	t->setData( 1, Qt::UserRole, valueext );
	parameters->addTopLevelItem( t );
}



CertificateDialog::CertificateDialog( QWidget *parent )
:	QDialog( parent )
,	d( new CertificateDialogPrivate )
{
	d->setupUi( this );
	d->tabWidget->removeTab( 2 );
}

CertificateDialog::CertificateDialog( const QSslCertificate &cert, QWidget *parent )
:	QDialog( parent )
,	d( new CertificateDialogPrivate )
{
	d->setupUi( this );
	setCertificate( cert );
	d->tabWidget->removeTab( 2 );
}

CertificateDialog::~CertificateDialog() { delete d; }

void CertificateDialog::on_parameters_itemSelectionChanged()
{
	const QList<QTreeWidgetItem*> &list = d->parameters->selectedItems();
	if( !list.isEmpty() )
		d->parameterContent->setPlainText( list[0]->data( 1,
			list[0]->data( 1, Qt::UserRole ).isNull() ? Qt::DisplayRole : Qt::UserRole ).toString() );
}

void CertificateDialog::save()
{
	QString file = QFileDialog::getSaveFileName( this,
		tr("Save certificate"),
		QString( "%1%2%3.cer" )
			.arg( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) )
			.arg( QDir::separator() )
			.arg( d->cert.subjectInfo( "serialNumber" ) ),
		tr("Certificates (*.cer *.crt *.pem)") );
	if( file.isEmpty() )
		return;

	QFile f( file );
	if( f.open( QIODevice::WriteOnly ) )
		f.write( d->cert.toPem() );
	else
		QMessageBox::warning( this, tr("Save certificate"), tr("Failed to save file") );
}

void CertificateDialog::setCertificate( const QSslCertificate &cert )
{
	d->cert = cert;
	SslCertificate c = cert;
	QString i;
	QTextStream s( &i );
	s << "<b>" << tr("Certificate Information") << "</b><br />";
	s << "<hr>";
	s << "<b>" << tr("This certificate is intended for following purpose(s):") << "</b>";
	s << "<ul>";
	Q_FOREACH( const QString &ext, c.enhancedKeyUsage() )
		s << "<li>" << ext << "</li>";
	s << "</ul>";
	s << "<br /><br /><br /><br />";
	//s << tr("* Refer to the certification authority's statement for details.") << "<br />";
	s << "<hr>";
	s << "<p style='margin-left: 30px;'>";
	s << "<b>" << tr("Issued to:") << "</b> " << c.subjectInfo( QSslCertificate::CommonName );
	s << "<br /><br /><br />";
	s << "<b>" << tr("Issued by:") << "</b> " << c.issuerInfo( QSslCertificate::CommonName );
	s << "<br /><br /><br />";
	s << "<b>" << tr("Valid from") << "</b> " << c.effectiveDate().toLocalTime().toString( "dd.MM.yyyy" ) << " ";
	s << "<b>" << tr("to") << "</b> "<< c.expiryDate().toLocalTime().toString( "dd.MM.yyyy" );
	s << "</p>";
	d->info->setHtml( i );

	d->addItem( tr("Version"), "V" + c.version() );
	d->addItem( tr("Serial number"), QString( "%1 (0x%2)" )
		.arg( c.serialNumber().constData() )
		.arg( c.serialNumber( true ).constData() ) );
	d->addItem( tr("Signature algorithm"), c.signatureAlgorithm() );

	QStringList text, textExt;
	Q_FOREACH( const QByteArray &subject, QList<QByteArray>() << "CN" << "OU" << "O" << "C" )
	{
		const QString &data = c.issuerInfo( subject );
		if( data.isEmpty() )
			continue;
		text << data;
		textExt << QString( "%1 = %2" ).arg( subject.constData() ).arg( data );
	}
	d->addItem( tr("Issuer"), text.join( ", " ), textExt.join( "\n" ) );
	d->addItem( tr("Valid from"), DateTime( c.effectiveDate().toLocalTime() ).toStringZ( "dd.MM.yyyy hh:mm:ss" ) );
	d->addItem( tr("Vaild to"), DateTime( c.expiryDate().toLocalTime() ).toStringZ( "dd.MM.yyyy hh:mm:ss" ) );

	text.clear();
	textExt.clear();
	Q_FOREACH( const QByteArray &subject,
		QList<QByteArray>() << "serialNumber" << "GN" << "SN" << "CN" << "OU" << "O" << "C" )
	{
		const QString &data = c.subjectInfo( subject );
		if( data.isEmpty() )
			continue;
		text << data;
		textExt << QString( "%1 = %2" ).arg( subject.constData() ).arg( data );
	}
	d->addItem( tr("Subject"), text.join( ", " ), textExt.join( "\n" ) );
	d->addItem( tr("Public key"), c.keyName(), c.publicKeyHex() );

	QStringList enhancedKeyUsage = c.enhancedKeyUsage().values();
	if( !enhancedKeyUsage.isEmpty() )
		d->addItem( tr("Enhanched key usage"), enhancedKeyUsage.join( ", " ), enhancedKeyUsage.join( "\n" ) );
	QStringList policies = c.policies();
	if( !policies.isEmpty() )
		d->addItem( tr("Certificate policies"), policies.join( ", " ) );
	d->addItem( tr("Authority key identifier"), c.toHex( c.authorityKeyIdentifier() ) );
	d->addItem( tr("Subject key identifier"), c.toHex( c.subjectKeyIdentifier() ) );
	QStringList keyUsage = c.keyUsage().values();
	if( !keyUsage.isEmpty() )
		d->addItem( tr("Key usage"), keyUsage.join( ", " ), keyUsage.join( "\n" ) );

	d->parameters->header()->setResizeMode( 0, QHeaderView::ResizeToContents );
}
