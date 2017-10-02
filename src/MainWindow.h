/*
 * QEstEidUtil
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

#include <QtWidgets/QWidget>

#define THREE_ATTEMPTS	3		// user has three attempts to enter a correct PIN1/PIN2/PUK code

class MainWindowPrivate;

class MainWindow: public QWidget
{
	Q_OBJECT

public:
	enum ButtonTypes
	{
		PageEmpty = 0x00,

		PageCert = 0x01,
		PageCertAuthView = 0x10 | PageCert,
		PageCertSignView = 0x20 | PageCert,
		PageCertPin1 = 0x30 | PageCert,
		PageCertPin2 = 0x40 | PageCert,
		PageCertUpdate = 0x50 | PageCert,

		PageEmail = 0x02,
		PageEmailStatus = 0x10 | PageEmail,
		PageEmailActivate = 0x20 | PageEmail,

		PagePukInfo = 0x03,

		PagePin1Pin = 0x04,
		PagePin1Puk = 0x10 | PagePin1Pin,
		PagePin1Unblock = 0x20 | PagePin1Pin,
		PagePin1ChangePin = 0x30 | PagePin1Pin,
		PagePin1ChangePuk = 0x40 | PagePin1Pin,
		PagePin1ChangeUnblock = 0x50 | PagePin1Pin,

		PagePin2Pin = 0x05,
		PagePin2Puk = 0x10 | PagePin2Pin,
		PagePin2Unblock = 0x20 | PagePin2Pin,
		PagePin2ChangePin = 0x30 | PagePin2Pin,
		PagePin2ChangePuk = 0x40 | PagePin2Pin,
		PagePin2ChangeUnblock = 0x50 | PagePin2Pin,

		PagePuk = 0x06,
		PagePukChange = 0x10 | PagePuk
	};

	MainWindow( QWidget *parent = 0 );
	~MainWindow();


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
	bool eventFilter(QObject *obj, QEvent *event);
	MainWindowPrivate *d;
	QString lang;

	Q_DECLARE_PRIVATE_D(d, MainWindow)
};
