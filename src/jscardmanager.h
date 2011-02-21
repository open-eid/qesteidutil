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

#pragma once

#include "jsesteidcard.h"

#include <QObject>
#include <QHash>
#include <QTimer>

#include <smartcardpp/common.h>
#include <smartcardpp/PCSCManager.h>

#include <fstream>

class JsCardManager : public QObject
{
    Q_OBJECT

    struct ReaderState {
		QString name;
        bool connected;
		QString cardId;
		int id;
		QString state;
    };

public:
    JsCardManager(JsEsteidCard *jsEsteidCard);

public slots:
    int getReaderCount();
    QString getReaderName( int i );
    bool selectReader( int i );
	bool selectReader( const ReaderState &reader );
	bool isInReader( const QString &cardId );
	bool isInReader( int readerNum );
	QString activeCardId();
	QString cardId( int readerNum );
	void showDiagnostics();
	void findCard();
	bool anyCardsInReader();
	int activeReaderNum();
    void allowRead();
	void disableRead();
	void newManager();

	bool checkCerts();

private slots:
    void pollCard();

signals:
    void cardEvent(QString func, int i);
    void cardError(QString func, QString err);

private:
	PCSCManager *cardMgr;
	JsEsteidCard *m_jsEsteidCard;
	QTimer pollTimer;
	std::ofstream log;

	QHash<QString,ReaderState> cardReaders;

	void handleError(QString msg);
	bool readAllowed;
};
