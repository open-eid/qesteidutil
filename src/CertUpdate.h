/*
 * QEstEidUtil
 *
 * Copyright (C) 2009,2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009,2010 Raul Metsma <raul@innovaatik.ee>
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

#include <QObject>
#include <QTcpSocket>
#include <QUrl>

#include "jscardmanager.h"
#include "smartcardpp/esteid/EstEidCard.h"

class CertUpdate: public QObject
{
	Q_OBJECT

public:

	CertUpdate( int reader, QObject *parent = 0 );
	~CertUpdate();

	bool checkUpdateAllowed();
	void startUpdate();

private:
	void throwError( const QString &msg );

	QString m_pin;
	EstEidCard *card;
	PCSCManager cardMgr;
	QTcpSocket *sock;
	char challenge[8];
	char personCode[11], documentNumber[8], tmpResult[36], certInfo[33][114];
	int step, serverStep, authKey, signKey;
	bool generateKeys;
	QByteArray step10;
	
	bool checkConnection() const;
	QByteArray runStep( int step, QByteArray result = "" );
	QByteArray queryServer( int step, QByteArray result );
	JsCertData *m_authCert;
	QUrl updateUrl;
};
