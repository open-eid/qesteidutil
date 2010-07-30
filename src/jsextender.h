/*
 * QEstEidUtil
 *
 * Copyright (C) 2009 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009 Raul Metsma <raul@innovaatik.ee>
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

#include "../common/sslConnect.h"

#include <QLabel>
#include <QObject>
#include <QWebFrame>
#include <QDateTime>
#include <QHttp>
#include <QXmlStreamReader>

class MainWindow;
class SettingsDialog;

class JsExtender : public QObject
{
    Q_OBJECT

public:
	enum MobileResult {
		OK = 0,
		NoCert = 1,
		NotActive = 2,
		NoIDCert = 3,
		InternalError = 100,
		InterfaceNotReady = 101
	};
    JsExtender( MainWindow* );
	~JsExtender();
    void registerObject( const QString &name, QObject *object );

private:
	MainWindow *m_mainWindow;
    QMap<QString, QObject *> m_registeredObjects;
	QString m_tempFile;
	QXmlStreamReader xml;
	QString m_locale;
	QLabel *m_loading;
	QByteArray getUrl( SSLConnect::RequestType, const QString &def );
	QHttp m_http;

public slots:
	void setLanguage( const QString &lang );
    void javaScriptWindowObjectCleared();
    QVariant jsCall( const QString &function, int argument );
    QVariant jsCall( const QString &function, const QString &argument );
	QVariant jsCall( const QString &function, const QString &argument, const QString &argument2 );
	void openUrl( const QString &url );
	
	void loadEmails();
	void activateEmail( const QString &email );
	QString readEmailAddresses();
	QString readForwards();
	
	void loadPicture();
	void savePicture();

	QString locale() { return m_locale; }
	void showSettings();

	void showLoading( const QString & );
	void closeLoading();

	void getMidStatus();
	void httpRequestFinished( int, bool error );

	void showMessage( const QString &type, const QString &message, const QString &title = "" );

	bool updateCertAllowed();
	bool updateCert();
};
