/*
 * QEstEidCommon
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

#include "PinDialog.h"

#include "SslCertificate.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExpValidator>
#include <QTimeLine>
#include <QVBoxLayout>

PinDialog::PinDialog( QWidget *parent )
:	QDialog( parent )
{}

PinDialog::PinDialog( PinFlags flags, const TokenData &t, QWidget *parent )
{
	SslCertificate c = t.cert();
	init( flags, c.toString( c.showCN() ? "CN serialNumber" : "GN SN serialNumber" ), t.flags() );
}

PinDialog::PinDialog( PinFlags flags, const QSslCertificate &cert, TokenData::TokenFlags token, QWidget *parent )
:	QDialog( parent )
{
	SslCertificate c = cert;
	init( flags, c.toString( c.isTempel() ? "CN serialNumber" : "GN SN serialNumber" ), token );
}

PinDialog::PinDialog( PinFlags flags, const QString &title, TokenData::TokenFlags token, QWidget *parent )
:	QDialog( parent )
{ init( flags, title, token ); }

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
		text += tr("Selected action requires sign certificate.") + "<br />" +
			(flags & PinpadFlag ?
				tr("For using sign certificate enter PIN2 with pinpad") :
				tr("For using sign certificate enter PIN2") );
		regexp.setPattern( "\\d{5,12}" );
	}
	else
	{
		_title = tr("Authendicating") + " - " + title;
		text += tr("Selected action requires auth certificate.") + "<br />" +
			(flags & PinpadFlag ?
				tr("For using auth certificate enter PIN1 with pinpad") :
				tr("For using auth certificate enter PIN1") );
		regexp.setPattern( "\\d{4,12}" );
	}
	setWindowTitle( _title );
	label->setText( text );

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
	else
	{
		m_text = new QLineEdit( this );
		m_text->setEchoMode( QLineEdit::Password );
		m_text->setFocus();
		m_text->setValidator( new QRegExpValidator( regexp, m_text ) );
		connect( m_text, SIGNAL(textEdited(QString)), SLOT(textEdited(QString)) );
		l->addWidget( m_text );

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
