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

#include "PinDialog.h"

#include "Common.h"
#include "SslCertificate.h"

#include <QtCore/QTimeLine>
#include <QtGui/QRegExpValidator>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#else
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#endif

PinDialog::PinDialog( PinFlags flags, const TokenData &t, QWidget *parent )
:	QDialog( parent )
{
	SslCertificate c = t.cert();
	init( flags, c.toString( c.showCN() ? "CN serialNumber" : "GN SN serialNumber" ), t.flags() );
}

PinDialog::PinDialog( PinFlags flags, const QSslCertificate &cert, TokenData::TokenFlags token, QWidget *parent )
:	QDialog( parent )
{
	SslCertificate c = cert;
	init( flags, c.toString( c.showCN() ? "CN serialNumber" : "GN SN serialNumber" ), token );
}

void PinDialog::init( PinFlags flags, const QString &title, TokenData::TokenFlags token )
{
	setMinimumWidth( 350 );
	setWindowModality( Qt::ApplicationModal );

	QLabel *label = new QLabel( this );
	QVBoxLayout *l = new QVBoxLayout( this );
	l->addWidget( label );

	QString _title = title;
	QString text;

	if( token & TokenData::PinFinalTry )
		text += "<font color='red'><b>" + tr("PIN will be locked next failed attempt") + "</b></font><br />";
	else if( token & TokenData::PinCountLow )
		text += "<font color='red'><b>" + tr("PIN has been entered incorrectly one time") + "</b></font><br />";

	text += QString( "<b>%1</b><br />" ).arg( title );
	if( flags & Pin2Type )
	{
		_title = tr("Signing") + " - " + title;
		QString t = flags & PinpadFlag ?
			tr("For using sign certificate enter PIN2 at the reader") :
			tr("For using sign certificate enter PIN2");
		text += tr("Selected action requires sign certificate.") + "<br />" + t;
		regexp.setPattern( "\\d{5,12}" );
	}
	else
	{
		_title = tr("Authentication") + " - " + title;
		QString t = flags & PinpadFlag ?
			tr("For using authentication certificate enter PIN1 at the reader") :
			tr("For using authentication certificate enter PIN1");
		text += tr("Selected action requires authentication certificate.") + "<br />" + t;
		regexp.setPattern( "\\d{4,12}" );
	}
	setWindowTitle( _title );
	label->setText( text );
	Common::setAccessibleName( label );

	if( flags & PinpadFlag )
	{
		setWindowFlags( (windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint );
		QProgressBar *progress = new QProgressBar( this );
		progress->setRange( 0, 30 );
		progress->setValue( progress->maximum() );
		progress->setTextVisible( false );
		l->addWidget( progress );
		QTimeLine *statusTimer = new QTimeLine( progress->maximum() * 1000, this );
		statusTimer->setCurveShape( QTimeLine::LinearCurve );
		statusTimer->setFrameRange( progress->maximum(), progress->minimum() );
		connect( statusTimer, SIGNAL(frameChanged(int)), progress, SLOT(setValue(int)) );
		connect( this, SIGNAL(startTimer()), statusTimer, SLOT(start()) );
	}
	else if( !(flags & PinpadNoProgressFlag) )
	{
		m_text = new QLineEdit( this );
		m_text->setEchoMode( QLineEdit::Password );
		m_text->setFocus();
		m_text->setValidator( new QRegExpValidator( regexp, m_text ) );
		m_text->setMaxLength( 12 );
		connect( m_text, SIGNAL(textEdited(QString)), SLOT(textEdited(QString)) );
		l->addWidget( m_text );
		label->setBuddy( m_text );

		QDialogButtonBox *buttons = new QDialogButtonBox(
			QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal, this );
		ok = buttons->button( QDialogButtonBox::Ok );
		ok->setAutoDefault( true );
		connect( buttons, SIGNAL(accepted()), SLOT(accept()) );
		connect( buttons, SIGNAL(rejected()), SLOT(reject()) );
		l->addWidget( buttons );

		textEdited( QString() );
	}
}

QString PinDialog::text() const { return m_text->text(); }

void PinDialog::textEdited( const QString &text )
{ ok->setEnabled( regexp.exactMatch( text ) ); }
