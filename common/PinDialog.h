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

#pragma once

#include <QDialog>

#include "TokenData.h"

#include <QRegExp>

class QLineEdit;
class QSslCertificate;

class PinDialog: public QDialog
{
	Q_OBJECT
public:
	enum PinFlags
	{
		Pin1Type = 0,
		Pin2Type = 1,
		PinpadFlag = 2,
		Pin1PinpadType = Pin1Type|PinpadFlag,
		Pin2PinpadType = Pin2Type|PinpadFlag,
	};
	PinDialog( PinFlags flags, const TokenData &t, QWidget *parent = 0 );
	PinDialog( PinFlags flags, const QSslCertificate &cert, TokenData::TokenFlags token, QWidget *parent = 0 );

	QString text() const;

signals:
	void startTimer();

private Q_SLOTS:
	void textEdited( const QString &text );

private:
	void init( PinFlags flags, const QString &title, TokenData::TokenFlags token );

	QLineEdit	*m_text;
	QPushButton	*ok;
	QRegExp		regexp;
};
