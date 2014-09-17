/*
 * QEstEidUtil
 *
 * Copyright (C) 2011-2013 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2011-2013 Raul Metsma <raul@innovaatik.ee>
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

#include <QtCore/QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif

class MainWindowPrivate;
class QSmartCard;

class MainWindow: public QWidget
{
    Q_OBJECT

public:
	enum ButtonTypes
	{
		PageEmpty = 0x00,

		PageCert = 0x01,
		PageCertAuthView = 0x11,
		PageCertSignView = 0x21,
		PageCertPin1 = 0x31,
		PageCertPin2 = 0x41,
		PageCertUpdate = 0x51,

		PageEmail = 0x02,
		PageEmailStatus = 0x12,
		PageEmailActivate = 0x22,

		PageMobile = 0x03,
		PageMobileStatus = 0x13,
		PageMobileActivate = 0x23,

		PagePukInfo = 0x04,

		PagePin1Pin = 0x05,
		PagePin1Puk = 0x15,
		PagePin1Unblock = 0x25,
		PagePin1ChangePin = 0x35,
		PagePin1ChangePuk = 0x45,
		PagePin1ChangeUnblock = 0x55,

		PagePin2Pin = 0x06,
		PagePin2Puk = 0x16,
		PagePin2Unblock = 0x26,
		PagePin2ChangePin = 0x36,
		PagePin2ChangePuk = 0x46,
		PagePin2ChangeUnblock = 0x56,

		PagePuk = 0x07,
		PagePukChange = 0x17
	};

	MainWindow( QWidget *parent = 0 );
	~MainWindow();

	QSmartCard* smartCard() const;

public slots:
	void raiseAndRead();

private slots:
	void on_languages_activated( int index );
	void pageButtonClicked();
	void loadPicture();
	void savePicture();
	void setDataPage( int index );
	void showAbout();
	void showDiagnostics();
	void showHelp();
	void showSettings();
	void showWarning( const QString &msg );
	void updateData();

private:
	MainWindowPrivate *d;
	QString lang;

	Q_DECLARE_PRIVATE_D(d, MainWindow)
};
