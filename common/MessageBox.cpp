/*
 * QEstEidCommon
 *
 * Copyright (C) 2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010 Raul Metsma <raul@innovaatik.ee>
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

#include "MessageBox.h"

#include <QAbstractButton>
#include <QDebug>
#include <QDialogButtonBox>
#include <QEvent>
#include <QTextEdit>

DMessageBox::DMessageBox( QWidget *parent )
: QMessageBox( parent )
{
	if( (b = findChild<QDialogButtonBox*>()) )
		connect( b, SIGNAL(clicked(QAbstractButton*)), SLOT(fixDetailsLabel()) );
}

DMessageBox::DMessageBox( Icon icon, const QString &title, const QString &text,
	StandardButtons buttons, QWidget *parent, Qt::WindowFlags flags )
: QMessageBox( icon, title, text, buttons, parent, flags )
{
	if( (b = findChild<QDialogButtonBox*>()) )
		connect( b, SIGNAL(clicked(QAbstractButton*)), SLOT(fixDetailsLabel()) );
}

bool DMessageBox::event( QEvent *e )
{
	bool result = QMessageBox::event( e );
	if( e->type() == QEvent::LanguageChange )
		fixDetailsLabel();
	return result;
}

void DMessageBox::fixDetailsLabel()
{
	if( !b )
		return;
	QWidget *details = 0;
	if( QTextEdit *w = findChild<QTextEdit*>() )
		details = w->parentWidget();
	if( !details )
		return;
	Q_FOREACH( QAbstractButton *button, b->buttons() )
	{
		if( b->buttonRole( button ) == QDialogButtonBox::ActionRole )
		{
			button->setText( details->isVisible() ? tr("Hide Details...") : tr("Show Details...") );
			button->setMinimumSize( button->sizeHint() + QSize( 20, 0 ) );
		}
	}
	adjustSize();
}
