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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "QSmartCard.h"
#include "QSmartCard_p.h"
#ifdef Q_OS_WIN
#include "CertStore.h"
#include "SettingsDialog.h"
#endif
#include "sslConnect.h"
#include "Updater.h"
#include "XmlReader.h"

#include <common/AboutDialog.h>
#include <common/CertificateWidget.h>
#include <common/Common.h>
#include <common/Configuration.h>
#include <common/DateTime.h>
#include <common/QPCSC.h>
#include <common/Settings.h>
#include <common/SslCertificate.h>
#ifdef Q_OS_MAC
#include <common/MacMenuBar.h>
#else
class MacMenuBar;
#endif

#include <QtCore/QDate>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtCore/QTranslator>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

Q_DECLARE_METATYPE(MobileStatus)
Q_DECLARE_METATYPE(Emails)

class MainWindowPrivate: public Ui::MainWindow
{
	Q_DECLARE_TR_FUNCTIONS(MainWindow)
	Q_DECLARE_PUBLIC(::MainWindow)
public:

	void clearPins();
	void hideLoading();
	QByteArray sendRequest( SSLConnect::RequestType type, const QString &param = QString() );
	void showLoading( const QString &text );
	void showWarning( const QString &msg, const QString &details = QString() );
	void updateMobileStatusText( const QVariant &data, bool set );
	bool validateCardError( QSmartCardData::PinType type, int flags, QSmartCard::ErrorType err );
	bool validatePin( QSmartCardData::PinType type, bool puk, const QString &old, const QString &pin, const QString &pin2 );

	::MainWindow *q_ptr = nullptr;
	QTranslator appTranslator, qtTranslator, commonTranslator;
	MacMenuBar *bar = nullptr;
	QSmartCard *smartcard = nullptr;
	QLabel *loading = nullptr;
	QPushButton *loadPicture = nullptr, *savePicture = nullptr;
	QButtonGroup *b = nullptr;
};



void MainWindowPrivate::clearPins()
{
	changePin1Repeat->clear();
	changePin1New->clear();
	changePin1Validate->clear();
	changePin2Repeat->clear();
	changePin2New->clear();
	changePin2Validate->clear();
	changePukRepeat->clear();
	changePukNew->clear();
	changePukValidate->clear();
	switch( dataWidget->currentIndex() )
	{
	case ::MainWindow::PagePin1Pin: changePin1Validate->setFocus(); break;
	case ::MainWindow::PagePin2Pin: changePin2Validate->setFocus(); break;
	case ::MainWindow::PagePuk: changePukValidate->setFocus(); break;
	default: break;
	}
}

void MainWindowPrivate::hideLoading()
{
	loading->hide();
	loading->parentWidget()->setEnabled( true );
}

QByteArray MainWindowPrivate::sendRequest( SSLConnect::RequestType type, const QString &param )
{
	Q_Q(::MainWindow);
	switch( type )
	{
	case SSLConnect::ActivateEmails: showLoading(  tr("Activating e-mail settings") ); break;
	case SSLConnect::EmailInfo: showLoading( tr("Loading e-mail settings") ); break;
	case SSLConnect::MobileInfo: showLoading( tr("Requesting Mobiil-ID status") ); break;
	case SSLConnect::PictureInfo: showLoading( tr("Loading picture") ); break;
	default: showLoading( tr( "Loading data" ) ); break;
	}

	if( !validateCardError( QSmartCardData::Pin1Type, type, smartcard->login( QSmartCardData::Pin1Type ) ) )
		return QByteArray();

	SSLConnect ssl;
	ssl.setToken( smartcard->data().authCert(), smartcard->key() );
	QByteArray buffer = ssl.getUrl( type, param );
	smartcard->logout();
	q->updateData();
	if( !ssl.errorString().isEmpty() )
	{
		switch( type )
		{
		case SSLConnect::ActivateEmails: showWarning( tr("Failed activating e-mail forwards."), ssl.errorString() ); break;
		case SSLConnect::EmailInfo: showWarning( tr("Failed loading e-mail settings."), ssl.errorString() ); break;
		case SSLConnect::MobileInfo: showWarning( tr("Failed loading Mobiil-ID settings."), ssl.errorString() ); break;
		case SSLConnect::PictureInfo: showWarning( tr("Loading picture failed."), ssl.errorString() ); break;
		default: showWarning( tr("Failed to load data"), ssl.errorString() ); break;
		}
		return QByteArray();
	}
	return buffer;
}

void MainWindowPrivate::showLoading( const QString &text )
{
	Q_Q(::MainWindow);
	loading->setFixedSize( text.size() > 20 ? 300 : 250, 100 );
	loading->move( q->width()/2 - loading->width()/2, 305 );
	loading->setText( text );
	loading->show();
	loading->parentWidget()->setDisabled( true );
}

void MainWindowPrivate::showWarning( const QString &msg, const QString &details )
{
	QMessageBox d( QMessageBox::Warning, tr("ID-card utility"), msg, QMessageBox::Close, qApp->activeWindow() );
	d.setWindowModality( Qt::WindowModal );
	if( !details.isEmpty() )
	{
		//d.addButton( QMessageBox::Help );
		d.setDetailedText( details );
	}
	if( d.exec() == QMessageBox::Help )
		Common::showHelp( msg );
}

void MainWindowPrivate::updateMobileStatusText( const QVariant &data, bool set )
{
	if( set )
		mobileStatus->setProperty( "STATUS", data );

	if( mobileStatus->property("STATUS").isNull() )
		return mobileStatus->clear();
	MobileStatus mobile = mobileStatus->property("STATUS").value<MobileStatus>();
	QString text;
	QTextStream s( &text );
	s << mobile.value( "MSISDN" ) << "<br />";
	s << tr("Mobile operator") << ": " << mobile.value( "Operator" ) << "<br />";
	s << tr("Mobile status") << ": ";
	if( mobile.value( "Status" ) == "Active" )
		s << "<span style='color: #509b00'>"
			<< XmlReader::mobileStatus( mobile.value( "Status" ) ) << "</span><br />"
			<< tr("Certificates are valid till") << ": " << mobile.value( "MIDCertsValidTo" );
	else
		s << "<span style='color: #e80303'>"
			<< XmlReader::mobileStatus( mobile.value( "Status" ) ) << "</span>";
	mobileStatus->setText( text );
	Common::setAccessibleName( mobileStatus );
}

bool MainWindowPrivate::validateCardError( QSmartCardData::PinType type, int flags, QSmartCard::ErrorType err )
{
	Q_Q(::MainWindow);
	q->updateData();
	QSmartCardData::PinType t = flags == 1025 ? QSmartCardData::PukType : type;
	switch( err )
	{
	case QSmartCard::NoError: return true;
	case QSmartCard::CancelError: break;
	case QSmartCard::BlockedError:
		showWarning( tr("%1 blocked").arg( QSmartCardData::typeString( t ) ) );
		q->setDataPage( ::MainWindow::PageCert );
		break;
	case QSmartCard::DifferentError:
		showWarning( tr("New %1 codes doesn't match").arg( QSmartCardData::typeString( type ) ) ); break;
	case QSmartCard::LenghtError:
		switch( type )
		{
		case QSmartCardData::Pin1Type: showWarning( tr("PIN1 length has to be between 4 and 12") ); break;
		case QSmartCardData::Pin2Type: showWarning( tr("PIN2 length has to be between 5 and 12") ); break;
		case QSmartCardData::PukType: showWarning( tr("PUK length has to be between 8 and 12") ); break;
		}
		break;
	case QSmartCard::OldNewPinSameError:
		showWarning( tr("Old and new %1 has to be different!").arg( QSmartCardData::typeString( type ) ) );
		break;
	case QSmartCard::ValidateError:
		showWarning( tr("Wrong %1 code. You can try %n more time(s).", "",
			smartcard->data().retryCount( t ) ).arg( QSmartCardData::typeString( t ) ) );
		break;
	default:
		switch( flags )
		{
		case SSLConnect::ActivateEmails: showWarning( tr("Failed activating e-mail forwards.") ); break;
		case SSLConnect::EmailInfo: showWarning( tr("Failed loading e-mail settings.") ); break;
		case SSLConnect::MobileInfo: showWarning( tr("Failed loading Mobiil-ID settings.") ); break;
		case SSLConnect::PictureInfo: showWarning( tr("Loading picture failed.") ); break;
		default:
			showWarning( tr("Changing %1 failed").arg( QSmartCardData::typeString( type ) ) ); break;
		}
		break;
	}
	return false;
}

bool MainWindowPrivate::validatePin( QSmartCardData::PinType type, bool puk,
	const QString &old, const QString &pin, const QString &pin2 )
{
	QString name = QSmartCardData::typeString( type );
	if( old.isEmpty() )
	{ showWarning( puk ? tr("Enter PUK code") : tr("Enter current %1 code").arg( name ) ); return false; }
	if( pin.isEmpty() )
	{ showWarning( tr("Enter new %1 code").arg( name ) ); return false; }
	if( pin2.isEmpty() )
	{ showWarning( tr("Retry your new %1 code").arg( name ) ); return false; }
	if( pin == old )
	{ showWarning( tr("Old and new %1 has to be different!").arg( name ) ); return false; }
	if( pin != pin2 )
	{ showWarning( tr("New %1 codes doesn't match").arg( name ) ); return false; }

	switch( type )
	{
	case QSmartCardData::Pin1Type:
		if( pin.size() < 4 || pin.size() > 12 )
		{ showWarning( tr("PIN1 length has to be between 4 and 12") ); return false; }
		break;
	case QSmartCardData::Pin2Type:
		if( pin.size() < 5 || pin.size() > 12 )
		{ showWarning( tr("PIN2 length has to be between 5 and 12") ); return false; }
		break;
	case QSmartCardData::PukType:
		if( pin.size() < 8 || pin.size() > 12 )
		{ showWarning( tr("PUK length has to be between 8 and 12") ); return false; }
		break;
	}

	QSmartCardData t = smartcard->data();
	QDate date = t.data( QSmartCardData::BirthDate ).toDate();
	if( !date.isNull() &&
		(pin.contains( date.toString( "yyyy" ) ) ||
		pin.contains( date.toString( "ddMM" ) ) ||
		pin.contains( date.toString( "MMdd" ) )) )
	{
		showWarning( tr("%1 have to be different than date of birth or year of birth").arg( name ) );
		return false;
	}
	return true;
}



MainWindow::MainWindow( QWidget *parent )
:	QWidget( parent )
,	d( new MainWindowPrivate )
{
	d->q_ptr = this;
	d->setupUi( this );
	setFixedSize( geometry().size() );
	const QList<QLabel*> labels{ d->emailInfo, d->mobileInfo, d->pukLocked,
		d->changePukInfo, d->changePin1InfoPinText, d->changePin2InfoPinText };
	for(QLabel *l: labels)
		Common::setAccessibleName( l );

	d->loading = new QLabel( d->centralwidget );
	d->loading->setObjectName( "loading" );
	d->loading->setAlignment( Qt::AlignCenter );
	d->loading->setWordWrap( true );
	d->loadPicture = new QPushButton( d->pictureFrame );
	d->loadPicture->setObjectName( "loadPicture" );
	d->loadPicture->setFlat( true );
	d->savePicture = new QPushButton( d->pictureFrame );
	d->savePicture->setObjectName( "savePicture" );
	d->savePicture->setFlat( true );
	QVBoxLayout *l = new QVBoxLayout( d->pictureFrame );
	l->setMargin( 0 );
	l->addSpacing( 0 );
	l->addWidget( d->loadPicture, 0, Qt::AlignCenter );
	l->addWidget( d->savePicture, 0, Qt::AlignBottom );

	setTabOrder( d->buttonPuk, d->loadPicture );
	setTabOrder( d->loadPicture, d->savePicture );

	QRegExpValidator *validator = new QRegExpValidator( QRegExp( "\\d*" ), this );
	d->changePin1Validate->setValidator( validator );
	d->changePin1New->setValidator( validator );
	d->changePin1Repeat->setValidator( validator );
	d->changePin2Validate->setValidator( validator );
	d->changePin2New->setValidator( validator );
	d->changePin2Repeat->setValidator( validator );
	d->changePukValidate->setValidator( validator );
	d->changePukNew->setValidator( validator );
	d->changePukRepeat->setValidator( validator );

	d->cards->hide();
	d->languages->setItemData( 0, "et" );
	d->languages->setItemData( 1, "en" );
	d->languages->setItemData( 2, "ru" );
	lang = "et";

	d->b = new QButtonGroup( this );
	// Cert page buttons
	d->b->addButton( d->authCert, PageCertAuthView );
	d->b->addButton( d->signCert, PageCertSignView );
	d->b->addButton( d->certUpdateButton, PageCertUpdate );
	d->b->addButton( d->authChangePin, PagePin1Pin );
	d->b->addButton( d->authRevoke, PagePin1Unblock );
	d->b->addButton( d->signChangePin, PagePin2Pin );
	d->b->addButton( d->signRevoke, PagePin2Unblock );
	// puk info buttons
	d->b->addButton( d->pukChange, PagePuk );
	d->b->addButton( d->pukLink1, PagePin1Puk );
	d->b->addButton( d->pukLink2, PagePin2Puk );
	// Email buttons
	d->b->addButton( d->checkEmail, PageEmailStatus );
	d->b->addButton( d->activateEmail, PageEmailActivate );
	// mobile buttons
	d->b->addButton( d->checkMobile, PageMobileStatus );
	d->b->addButton( d->mobileActivate, PageMobileActivate );
	// pin1 buttons
	d->b->addButton( d->changePin1InfoPinLink, PagePin1Puk );
	d->b->addButton( d->changePin1Cancel, PageCert );
	d->b->addButton( d->changePin1Change, PagePin1ChangePin );
	d->b->addButton( d->changePin1PinpadCancel, PageCert );
	d->b->addButton( d->changePin1PinpadChange, PagePin1ChangePin );
	//pin2 buttons
	d->b->addButton( d->changePin2InfoPinLink, PagePin2Puk );
	d->b->addButton( d->changePin2Cancel, PageCert );
	d->b->addButton( d->changePin2Change, PagePin2ChangePin );
	d->b->addButton( d->changePin2PinpadCancel, PageCert );
	d->b->addButton( d->changePin2PinpadChange, PagePin2ChangePin );
	//puk buttons
	d->b->addButton( d->changePukCancel, PageCert );
	d->b->addButton( d->changePukChange, PagePukChange );
	d->b->addButton( d->changePukPinpadCancel, PageCert );
	d->b->addButton( d->changePukPinpadChange, PagePukChange );

	connect( d->b, SIGNAL(buttonClicked(int)), SLOT(setDataPage(int)) );
	connect( d->loadPicture, SIGNAL(clicked()), SLOT(loadPicture()) );
	connect( d->savePicture, SIGNAL(clicked()), SLOT(savePicture()) );

	d->buttonCert->installEventFilter(this);
	qApp->installTranslator( &d->appTranslator );
	qApp->installTranslator( &d->qtTranslator );
	qApp->installTranslator( &d->commonTranslator );

#ifndef Q_OS_WIN
	d->headerLine1->hide();
	d->headerSettings->hide();
#else
	if(Settings(QSettings::SystemScope).value("disableSave", false).toBool())
	{
		d->headerLine1->hide();
		d->headerSettings->hide();
	}
#endif
#ifdef Q_OS_MAC
	d->bar = new MacMenuBar( false );
	if( d->headerSettings->isVisible() )
		d->bar->addAction( MacMenuBar::PreferencesAction, this, SLOT(showSettings()) );
	d->bar->addAction( MacMenuBar::AboutAction, this, SLOT(showAbout()) );
	d->bar->addAction( MacMenuBar::CloseAction, qApp, SLOT(quit()) );
#endif

	d->smartcard = new QSmartCard( this );
	connect( d->smartcard, SIGNAL(dataChanged()), SLOT(updateData()) );
	d->smartcard->start();
	connect( d->cards, SIGNAL(activated(QString)), d->smartcard, SLOT(selectCard(QString)), Qt::QueuedConnection );

	setDataPage( PageEmpty );
	int index = d->languages->findData( Settings::language() );
	if( index != -1 )
	{
		d->languages->setCurrentIndex( index );
		on_languages_activated( index );
	}
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_MAC
	delete d->bar;
#endif
	delete d;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if(obj != d->buttonCert || event->type() != QEvent::Paint || !d->certUpdate->property("updateEnabled").toBool())
		return false;
	static bool recursive = false;
	if(recursive)
		return false;
	recursive = true;
	QCoreApplication::sendEvent(obj, event);
	recursive = false;

	QRect r(0, 0, 12, 12);
	r.moveRight(d->buttonCert->rect().right() - 7);
	QPainter p(d->buttonCert);
	p.setRenderHints(QPainter::Antialiasing);
	p.setPen(Qt::red);
	p.setBrush(Qt::red);
	p.drawEllipse(r);
	p.setPen(Qt::white);
	p.drawText(r, Qt::AlignCenter, "!");
	return true;
}

void MainWindow::on_languages_activated( int index )
{
	lang = d->languages->itemData( index ).toString();
	if( lang == "en" ) QLocale::setDefault( QLocale( QLocale::English, QLocale::UnitedKingdom ) );
	else if( lang == "ru" ) QLocale::setDefault( QLocale( QLocale::Russian, QLocale::RussianFederation ) );
	else QLocale::setDefault( QLocale( QLocale::Estonian, QLocale::Estonia ) );

	d->appTranslator.load( ":/translations/" + lang );
	d->qtTranslator.load( ":/translations/qt_" + lang );
	d->commonTranslator.load( ":/translations/common_" + lang );
	d->retranslateUi( this );
	d->loadPicture->setText( tr("Load picture") );
	d->savePicture->setText( tr("save") );
	d->version->setText( windowTitle() + " " + qApp->applicationVersion() );

	if( d->changePin1Info->currentWidget() == d->changePin1InfoPin )
		d->changePin1ValidateLabel->setText( tr("Current PIN1 code") );
	else
		d->changePin1ValidateLabel->setText( tr("Current PUK code") );

	if( d->changePin2Info->currentWidget() == d->changePin2InfoPin )
		d->changePin2ValidateLabel->setText( tr("Current PIN2 code") );
	else
		d->changePin2ValidateLabel->setText( tr("Current PUK code") );

	updateData();
	d->updateMobileStatusText( QVariant(), false );
	if( !d->emailStatus->property( "FORWARDS" ).isNull() )
	{
		Emails emails = d->emailStatus->property( "FORWARDS" ).value<Emails>();
		QStringList text;
		for( Emails::const_iterator i = emails.constBegin(); i != emails.constEnd(); ++i )
		{
			text << QString( "%1 - %2 (%3)" )
				.arg( i.key() )
				.arg( i.value().first )
				.arg( i.value().second ? tr("active") : tr("not active") );
		}

		d->emailStatus->setText( text.join("<br />") );
	}
	if( !d->emailStatus->property( "STATUS" ).isNull() )
		d->emailStatus->setText( XmlReader::emailErr( d->emailStatus->property( "STATUS" ).toUInt() ) );
	Settings().setValue( "Main/Language", lang );
}

void MainWindow::pageButtonClicked()
{
	if( sender() == d->buttonCert ) setDataPage( PageCert );
	if( sender() == d->buttonEmail ) setDataPage( PageEmail );
	if( sender() == d->buttonMobile ) setDataPage( PageMobile );
	if( sender() == d->buttonPuk ) setDataPage( PagePukInfo );
}

void MainWindow::loadPicture()
{
	QByteArray buffer = d->sendRequest( SSLConnect::PictureInfo );
	d->hideLoading();
	if( buffer.isEmpty() )
		return;

	QPixmap pix;
	d->loadPicture->setHidden(pix.loadFromData(buffer));
	d->pictureFrame->setProperty( "PICTURE", pix );
	if( d->loadPicture->isVisible() )
	{
		XmlReader xml( buffer );
		QString error;
		xml.readEmailStatus( error );
		if( !error.isEmpty() )
			showWarning( XmlReader::emailErr( error.toUInt() ) );
		return;
	}
	d->pictureFrame->setPixmap( pix.scaled( 90, 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
	d->savePicture->setVisible(d->loadPicture->isHidden() &&
		!Settings(QSettings::SystemScope).value("disableSave", false).toBool());
}

void MainWindow::raiseAndRead()
{
	raise();
	showNormal();
	activateWindow();
	d->smartcard->reload();
}

void MainWindow::savePicture()
{
	QString file = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	file += "/" + d->smartcard->data().card();
	file = QFileDialog::getSaveFileName( this, tr("Save picture"), file,
		"JPEG (*.jpg *.jpeg);;PNG (*.png);;TIFF (*.tif *.tiff);;24-bit Bitmap (*.bmp)" );
	if( file.isEmpty() )
		return;
	QStringList exts = QStringList() << "png" << "jpg" << "jpeg" << "tiff" << "bmp";
	if( !exts.contains( QFileInfo( file ).suffix(), Qt::CaseInsensitive ) )
		file.append( ".jpg" );
	QPixmap pix = d->pictureFrame->property( "PICTURE" ).value<QPixmap>();
	if( !pix.save( file ) )
		showWarning( tr("Saving picture failed!") );
}

void MainWindow::setDataPage( int index )
{
	QSmartCardData t = d->smartcard->data();
	ButtonTypes page = ButtonTypes( index & 0x0f );
	d->dataWidget->setCurrentIndex( t.isNull() ? PageEmpty : page );
	d->buttonCert->setChecked( page == PageCert );
	d->buttonEmail->setChecked( page == PageEmail );
	d->buttonMobile->setChecked( page == PageMobile );
	d->buttonPuk->setChecked( page == PagePukInfo );
	if( t.isNull() )
		return;

	switch( index )
	{
	case PageCert:
		d->clearPins();
		d->loadPicture->setFocus();
		break;
	case PageCertAuthView:
		CertificateDialog( d->smartcard->data().authCert(), this ).exec();
		break;
	case PageCertSignView:
		CertificateDialog( d->smartcard->data().signCert(), this ).exec();
		break;
	case PageCertUpdate:
	{
#ifdef Q_OS_WIN
		CertStore s;
		s.remove(d->smartcard->data().authCert());
		s.remove(d->smartcard->data().signCert());
#endif
		d->showLoading( tr("Updating certificates") );
		d->smartcard->d->m.lock();
		Updater(d->smartcard->data().reader(), this).exec();
		d->smartcard->d->m.unlock();
		d->smartcard->reload();
		break;
	}
	case PageEmail:
		d->emailStatus->clear();
		d->emailStatus->setProperty( "STATUS", QVariant() );
		d->emailStatus->setProperty( "FORWARDS", QVariant() );
		d->activateEmailFrame->hide();
		d->checkEmailFrame->show();
		d->checkEmail->setFocus();
		break;
	case PageEmailActivate:
	{
		if( d->activateEmailAddress->text().isEmpty() || d->activateEmailAddress->text().indexOf( "@" ) == -1 )
		{
			d->showWarning( tr("E-mail address missing or invalid!") );
			break;
		}
		QByteArray buffer = d->sendRequest( SSLConnect::ActivateEmails, d->activateEmailAddress->text() );
		if( buffer.isEmpty() )
			break;
		XmlReader xml( buffer );
		QString error;
		xml.readEmailStatus( error );
		d->emailStatus->setText( XmlReader::emailErr( error.toUInt() ) );
		d->emailStatus->setProperty( "STATUS", error.toUInt() );
		d->emailStatus->setProperty( "FORWARDS", QVariant() );
		d->activateEmailFrame->hide();
		break;
	}
	case PageEmailStatus:
	{
		d->emailStatus->clear();
		d->emailStatus->setProperty( "STATUS", QVariant() );
		d->emailStatus->setProperty( "FORWARDS", QVariant() );
		QByteArray buffer = d->sendRequest( SSLConnect::EmailInfo );
		if( buffer.isEmpty() )
			break;
		XmlReader xml( buffer );
		QString error;
		QMultiHash<QString,QPair<QString,bool> > emails = xml.readEmailStatus( error );
		quint8 code = error.toUInt();
		if( emails.isEmpty() || code > 0 )
		{
			code = code ? code : 20;
			d->emailStatus->setText( XmlReader::emailErr( code ) );
			d->emailStatus->setProperty( "STATUS", code );
			if( code == 20 )
			{
				d->activateEmailFrame->show();
				d->activateEmailAddress->clear();
				d->activateEmailAddress->setFocus();
			}
		}
		else
		{
			QStringList text;
			for( Emails::const_iterator i = emails.constBegin(); i != emails.constEnd(); ++i )
			{
				text << QString( "%1 - %2 (%3)" )
					.arg( i.key() )
					.arg( i.value().first )
					.arg( i.value().second ? tr("active") : tr("not active") );
			}

			d->emailStatus->setText( text.join("<br />") );
			d->emailStatus->setProperty( "FORWARDS", QVariant::fromValue( emails ) );
		}
		d->checkEmailFrame->hide();
		break;
	}
	case PageMobile:
		d->updateMobileStatusText( QVariant(), true );
		d->checkMobileFrame->show();
		d->mobileActivateFrame->hide();
		d->checkMobile->setFocus();
		break;
	case PageMobileStatus:
	{
		d->updateMobileStatusText( QVariant(), true );
		QByteArray buffer = d->sendRequest( SSLConnect::MobileInfo );
		if( buffer.isEmpty() )
			break;
		XmlReader xml( buffer );
		int error = 0;
		MobileStatus mobile = xml.readMobileStatus( error );
		if( error )
		{
			showWarning( XmlReader::mobileErr( error ) );
			break;
		}
		d->updateMobileStatusText( QVariant::fromValue( mobile ), true );
		d->checkMobileFrame->hide();
		d->mobileActivateFrame->setVisible( mobile.value( "Status" ) != "Active" );
		break;
	}
	case PageMobileActivate:
		QDesktopServices::openUrl( tr("http://politsei.ee/en/teenused/isikut-toendavad-dokumendid/mobiil-id/") );
		break;
	case PagePin1Pin:
		d->changePin1Info->setCurrentWidget( d->changePin1InfoPin );
		d->changePin1PinpadInfo->setCurrentWidget( d->changePin1PinpadInfoPin );
		d->changePin1ValidateLabel->setText( tr("Current PIN1 code") );
		d->changePin1Validate->setFocus();
		d->changePin1Change->setText( tr("Change") );
		d->changePin1PinpadChange->setText( tr("Change with PinPad") );
		d->b->setId( d->changePin1Change, PagePin1ChangePin );
		d->b->setId( d->changePin1PinpadChange, PagePin1ChangePin );
		break;
	case PagePin1Puk:
		d->changePin1Info->setCurrentWidget( d->changePin1InfoPuk );
		d->changePin1PinpadInfo->setCurrentWidget( d->changePin1PinpadInfoPuk );
		d->changePin1ValidateLabel->setText( tr("Current PUK code") );
		d->changePin1Validate->setFocus();
		d->changePin1Change->setText( tr("Change") );
		d->changePin1PinpadChange->setText( tr("Change with PinPad") );
		d->b->setId( d->changePin1Change, PagePin1ChangePuk );
		d->b->setId( d->changePin1PinpadChange, PagePin1ChangePuk );
		break;
	case PagePin1Unblock:
		d->changePin1Info->setCurrentWidget( d->changePin1InfoUnblock );
		d->changePin1PinpadInfo->setCurrentWidget( d->changePin1PinpadInfoUnblock );
		d->changePin1ValidateLabel->setText( tr("Current PUK code") );
		d->changePin1Validate->setFocus();
		d->changePin1Change->setText( tr("Unblock") );
		d->changePin1PinpadChange->setText( tr("Unblock with PinPad") );
		d->b->setId( d->changePin1Change, PagePin1ChangeUnblock );
		d->b->setId( d->changePin1PinpadChange, PagePin1ChangeUnblock );
		break;
	case PagePin1ChangePin:
		if( !t.isPinpad() && !d->validatePin( QSmartCardData::Pin1Type, false,
				d->changePin1Validate->text(), d->changePin1New->text(), d->changePin1Repeat->text() ) )
		{
			d->clearPins();
			break;
		}
		if( t.isPinpad() )
			d->showLoading( tr("Enter PIN/PUK codes on PinPad") );
		else
			d->showLoading( tr("Changing %1 code").arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
		if( d->validateCardError( QSmartCardData::Pin1Type, 1024,
				d->smartcard->change( QSmartCardData::Pin1Type, d->changePin1New->text(), d->changePin1Validate->text() ) ) )
		{
			QMessageBox::information( this, windowTitle(), tr("%1 changed!").arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
			setDataPage( PageCert );
		}
		d->clearPins();
		break;
	case PagePin1ChangePuk:
	case PagePin1ChangeUnblock:
		if( !t.isPinpad() && !d->validatePin( QSmartCardData::Pin1Type, true,
				d->changePin1Validate->text(), d->changePin1New->text(), d->changePin1Repeat->text() ) )
		{
			d->clearPins();
			break;
		}

		if( !t.isPinpad() )
		{
			if( index == PagePin1ChangePuk )
				d->showLoading( tr("Changing %1 code")	.arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
			else
				d->showLoading( tr("Unblocking %1 code").arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
		}
		else
			d->showLoading( tr("Enter PIN/PUK codes on PinPad") );

		if( d->validateCardError( QSmartCardData::Pin1Type, 1025,
				d->smartcard->unblock( QSmartCardData::Pin1Type, d->changePin1New->text(), d->changePin1Validate->text() ) ) )
		{
			if( index == PagePin1ChangePuk )
				QMessageBox::information( this, windowTitle(), tr("%1 changed!")
					.arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
			else
				QMessageBox::information( this, windowTitle(), tr("%1 changed and your current certificates blocking has been removed!")
					.arg( QSmartCardData::typeString( QSmartCardData::Pin1Type ) ) );
			updateData();
			setDataPage( PageCert );
		}
		d->clearPins();
		break;
	case PagePin2Pin:
		d->changePin2Info->setCurrentWidget( d->changePin2InfoPin );
		d->changePin2PinpadInfo->setCurrentWidget( d->changePin2PinpadInfoPin );
		d->changePin2ValidateLabel->setText( tr("Current PIN2 code") );
		d->changePin2Validate->setFocus();
		d->changePin2Change->setText( tr("Change") );
		d->changePin2PinpadChange->setText( tr("Change with PinPad") );
		d->b->setId( d->changePin2Change, PagePin2ChangePin );
		d->b->setId( d->changePin2PinpadChange, PagePin2ChangePin );
		break;
	case PagePin2Puk:
		d->changePin2Info->setCurrentWidget( d->changePin2InfoPuk );
		d->changePin2PinpadInfo->setCurrentWidget( d->changePin2PinpadInfoPuk );
		d->changePin2ValidateLabel->setText( tr("Current PUK code") );
		d->changePin2Validate->setFocus();
		d->changePin2Change->setText( tr("Change") );
		d->changePin2PinpadChange->setText( tr("Change with PinPad") );
		d->b->setId( d->changePin2Change, PagePin2ChangePuk );
		d->b->setId( d->changePin2PinpadChange, PagePin2ChangePuk );
		break;
	case PagePin2Unblock:
		d->changePin2Info->setCurrentWidget( d->changePin2InfoUnblock );
		d->changePin2PinpadInfo->setCurrentWidget( d->changePin2PinpadInfoUnblock );
		d->changePin2ValidateLabel->setText( tr("Current PUK code") );
		d->changePin2Validate->setFocus();
		d->changePin2Change->setText( tr("Unblock") );
		d->changePin2PinpadChange->setText( tr("Unblock with PinPad") );
		d->b->setId( d->changePin2Change, PagePin2ChangeUnblock );
		d->b->setId( d->changePin2PinpadChange, PagePin2ChangeUnblock );
		break;
	case PagePin2ChangePin:
		if( !t.isPinpad() && !d->validatePin( QSmartCardData::Pin2Type, false,
				d->changePin2Validate->text(), d->changePin2New->text(), d->changePin2Repeat->text() ) )
		{
			d->clearPins();
			break;
		}
		if( t.isPinpad() )
			d->showLoading( tr("Enter PIN/PUK codes on PinPad") );
		else
			d->showLoading( tr("Changing %1 code").arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
		if( d->validateCardError( QSmartCardData::Pin2Type, 1024,
				d->smartcard->change( QSmartCardData::Pin2Type, d->changePin2New->text(), d->changePin2Validate->text() ) ) )
		{
			QMessageBox::information( this, windowTitle(), tr("%1 changed!").arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
			setDataPage( PageCert );
		}
		d->clearPins();
		break;
	case PagePin2ChangePuk:
	case PagePin2ChangeUnblock:
		if( !t.isPinpad() && !d->validatePin( QSmartCardData::Pin2Type, true,
				d->changePin2Validate->text(), d->changePin2New->text(), d->changePin2Repeat->text() ) )
		{
			d->clearPins();
			break;
		}

		if( !t.isPinpad() )
		{
			if( index == PagePin2ChangePuk )
				d->showLoading( tr("Changing %1 code").arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
			else
				d->showLoading( tr("Unblocking %1 code").arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
		}
		else
			d->showLoading( tr("Enter PIN/PUK codes on PinPad") );

		if( d->validateCardError( QSmartCardData::Pin2Type, 1025,
				d->smartcard->unblock( QSmartCardData::Pin2Type, d->changePin2New->text(), d->changePin2Validate->text() ) ) )
		{
			if( index == PagePin2ChangePuk )
				QMessageBox::information( this, windowTitle(), tr("%1 changed!")
					.arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
			else
				QMessageBox::information( this, windowTitle(), tr("%1 changed and your current certificates blocking has been removed!")
					.arg( QSmartCardData::typeString( QSmartCardData::Pin2Type ) ) );
			updateData();
			setDataPage( PageCert );
		}
		d->clearPins();
		break;
	case PagePuk:
		d->changePukValidate->setFocus();
		break;
	case PagePukChange:
		if( !t.isPinpad() && !d->validatePin( QSmartCardData::PukType, false,
				d->changePukValidate->text(), d->changePukNew->text(), d->changePukRepeat->text() ) )
		{
			d->clearPins();
			break;
		}
		if( t.isPinpad() )
			d->showLoading( tr("Enter PIN/PUK codes on PinPad") );
		else
			d->showLoading( tr("Changing %1 code").arg( QSmartCardData::typeString( QSmartCardData::PukType ) ) );
		if( d->validateCardError( QSmartCardData::PukType, 1024,
				d->smartcard->change( QSmartCardData::PukType, d->changePukNew->text(), d->changePukValidate->text() ) ) )
		{
			QMessageBox::information( this, windowTitle(), tr("%1 changed!").arg( QSmartCardData::typeString( QSmartCardData::PukType ) ) );
			setDataPage( PageCert );
		}
		d->clearPins();
		break;
	default: break;
	}
	d->hideLoading();
}

void MainWindow::showAbout()
{ (new AboutDialog( this ))->openTab( 0 ); }

void MainWindow::showDiagnostics()
{ (new AboutDialog( this ) )->openTab( 1 ); }

void MainWindow::showHelp()
{ QDesktopServices::openUrl( Common::helpUrl() ); }

void MainWindow::showSettings()
{
#if defined(Q_OS_WIN)
	if( d->headerSettings->isVisible() )
		(new SettingsDialog( d->smartcard->data(), this ) )->exec();
#endif
}

void MainWindow::showWarning( const QString &msg )
{ QMessageBox::warning( this, windowTitle(), msg ); }

void MainWindow::updateData()
{
	d->hideLoading();
	QSmartCardData t = d->smartcard->data();

	if( !t.isNull() )
	{
		QString text;
		QTextStream st( &text );

		st << "<font style='color: #54859b;'><font style='font-weight: bold; font-size: 16px;'>"
		   << tr("Card in reader") << " <font style='color: black;'>"
		   << t.data( QSmartCardData::DocumentId ).toString() << "</font></font><br />";
		if( t.authCert().type() & SslCertificate::EstEidType )
		{
			st << tr("This is");
			if( t.isValid() )
				st << " <font style='color: #509b00;'>" << tr("valid") << "</font> ";
			else
				st << " <font style='color: #e80303;'>" << tr("expired") << "</font> ";
			st << tr("document") << "<br />";
		}
		else
			st << tr("You're using Digital identity card") << "<br />";
		st << tr("Card is valid till") << " <font style='color: black;'>"
		   << DateTime( t.data( QSmartCardData::Expiry ).toDateTime() ).formatDate( "dd. MMMM yyyy" ) << "</font>";
		if( !t.isValid() )
			st << "<br /><font style='color: black;'>"
				<< tr("Instructions how to get a new document you can find "
					"<a href=\"http://www.politsei.ee/en/teenused/isikut-toendavad-dokumendid/id-kaart/taiskasvanule/\">here</a>")
				<< "</font>";
		st << "</font>";

		d->cardInfo->setAlignment( Qt::AlignVCenter|Qt::AlignLeft );
		d->cardInfo->setText( text );

		d->changePin1Stack->setCurrentIndex( t.isPinpad() );
		d->changePin2Stack->setCurrentIndex( t.isPinpad() );
		d->changePukStack->setCurrentIndex( t.isPinpad() );

		QStringList name = QStringList()
			<< SslCertificate::formatName( t.data( QSmartCardData::FirstName1 ).toString() )
			<< SslCertificate::formatName( t.data( QSmartCardData::FirstName2 ).toString() );
		name.removeAll( "" );
		d->personalName->setText( name.join(" ") );
		d->surName->setText( t.data( QSmartCardData::SurName ).toString() );
		d->personalCode->setText( t.data( QSmartCardData::Id ).toString() );
		d->personalBirth->setText( DateTime( t.data( QSmartCardData::BirthDate ).toDateTime() ).formatDate( "dd. MMMM yyyy" ) +
			(t.data( QSmartCardData::BirthPlace ).toString().isEmpty() ? "" : ", " + t.data( QSmartCardData::BirthPlace ).toString()) );
		d->personalCitizenLabel->setVisible( t.authCert().type() & SslCertificate::EstEidType );
		d->personalCitizen->setVisible( t.authCert().type() & SslCertificate::EstEidType );
		d->personalCitizen->setText( t.data( QSmartCardData::Citizen ).toString() );
		d->personalEmail->setText( t.data( QSmartCardData::Email ).toString() );
		const QList<QLabel*> list({ d->personalName, d->surName, d->personalCode, d->personalBirth, d->personalCitizen, d->personalEmail });
		for( QLabel *l: list )
			l->setToolTip( l->text() );
		d->personalEmail->setText( d->personalEmail->fontMetrics().elidedText(
			d->personalEmail->text(), Qt::ElideMiddle, d->personalEmail->width() ) );

		d->authTill->setText( DateTime( t.authCert().expiryDate().toLocalTime() ).formatDate( "dd. MMMM yyyy" ) );
		d->signTill->setText( DateTime( t.signCert().expiryDate().toLocalTime() ).formatDate( "dd. MMMM yyyy" ) );

		d->authInValidity->setVisible( t.retryCount( QSmartCardData::Pin1Type ) == 0 || !t.authCert().isValid() );
		d->authValidity->setVisible( t.retryCount( QSmartCardData::Pin1Type ) > 0 && t.authCert().isValid() );
		if ( t.retryCount( QSmartCardData::Pin1Type ) == 0 )
			d->authInValidity->setText( t.authCert().isValid() ? tr("valid but blocked") : tr("invalid and blocked") );
		else if( !t.authCert().isValid() )
			d->authInValidity->setText( tr("expired") );
		else
			d->authValidity->setText( tr("valid and applicable") );

		d->signInValidity->setVisible( t.retryCount( QSmartCardData::Pin2Type ) == 0 || !t.signCert().isValid() );
		d->signValidity->setVisible( t.retryCount( QSmartCardData::Pin2Type ) > 0 && t.signCert().isValid() );
		if( t.retryCount( QSmartCardData::Pin2Type ) == 0 )
			d->signInValidity->setText( t.signCert().isValid() ? tr("valid but blocked") : tr("invalid and blocked") );
		else if( !t.signCert().isValid() )
			d->signInValidity->setText( tr("expired") );
		else
			d->signValidity->setText( tr("valid and applicable") );

		d->authUsageCount->setText( tr( "Authentication key has been used %1 times" ).arg( t.usageCount( QSmartCardData::Pin1Type ) ) );
		d->signUsageCount->setText( tr( "Signature key has been used %1 times" ).arg( t.usageCount( QSmartCardData::Pin2Type ) ) );
		d->authUsageCount->setHidden( t.retryCount( QSmartCardData::Pin1Type ) == 0 );
		d->signUsageCount->setHidden( t.retryCount( QSmartCardData::Pin2Type ) == 0 );

		int authDays = std::max<int>( 0, QDateTime::currentDateTime().daysTo( t.authCert().expiryDate().toLocalTime() ) );
		int signDays = std::max<int>( 0, QDateTime::currentDateTime().daysTo( t.signCert().expiryDate().toLocalTime() ) );
		d->authCertExpired->setText( t.authCert().isValid() ?
			tr("Certificate will expire in %1 days").arg( authDays ) : tr("Certificate is expired") );
		d->signCertExpired->setText( t.signCert().isValid() ?
			tr("Certificate will expire in %1 days").arg( signDays ) : tr("Certificate is expired") );
		d->authCertExpired->setVisible( authDays <= 105 && t.retryCount( QSmartCardData::Pin1Type ) != 0 );
		d->signCertExpired->setVisible( signDays <= 105 && t.retryCount( QSmartCardData::Pin2Type ) != 0 );

		d->authChangePin->setVisible( t.retryCount( QSmartCardData::Pin1Type ) > 0 );
		d->signChangePin->setVisible( t.retryCount( QSmartCardData::Pin2Type ) > 0 );
		d->authCertBlocked->setHidden( t.retryCount( QSmartCardData::Pin1Type ) > 0 );
		d->signCertBlocked->setHidden( t.retryCount( QSmartCardData::Pin2Type ) > 0 );
		d->authRevoke->setVisible(
			t.retryCount( QSmartCardData::Pin1Type ) == 0 && t.retryCount( QSmartCardData::PukType ) > 0 );
		d->signRevoke->setVisible(
			t.retryCount( QSmartCardData::Pin2Type ) == 0 && t.retryCount( QSmartCardData::PukType ) > 0 );
		d->authFrame->setVisible( !t.authCert().isNull() );
		d->signFrame->setVisible( !t.signCert().isNull() );
		d->certsLine->setVisible( !t.authCert().isNull() || !t.signCert().isNull() );
		d->buttonEmail->setDisabled(t.version() == QSmartCardData::VER_USABLEUPDATER);
		d->buttonMobile->setDisabled(t.version() == QSmartCardData::VER_USABLEUPDATER);
		d->buttonPuk->setDisabled(t.version() == QSmartCardData::VER_USABLEUPDATER);

		d->certUpdate->setProperty("updateEnabled",
			Settings(qApp->applicationName()).value("updateButton", false).toBool() ||
			(
				t.retryCount( QSmartCardData::Pin1Type ) > 0 &&
				t.isValid() && (
					Configuration::instance().object().contains("EIDUPDATER-URL") ||
					(t.version() == QSmartCardData::VER_3_4 && Configuration::instance().object().contains("EIDUPDATER-URL-34")) ||
					(t.version() >= QSmartCardData::VER_3_5 && Configuration::instance().object().contains("EIDUPDATER-URL-35"))
				) && (
					!t.authCert().validateEncoding() ||
					!t.signCert().validateEncoding() ||
					t.version() & QSmartCardData::VER_HASUPDATER ||
					t.version() == QSmartCardData::VER_USABLEUPDATER
				)
			)
		);
		d->certUpdate->setVisible(d->certUpdate->property("updateEnabled").toBool());

		d->pukLocked->setVisible( t.retryCount( QSmartCardData::PukType ) == 0 );
		d->pukChange->setVisible( t.retryCount( QSmartCardData::PukType ) > 0 );
		d->pukLink1->setVisible( !t.isSecurePinpad() && t.retryCount( QSmartCardData::PukType ) > 0 );
		d->pukLink2->setVisible( !t.isSecurePinpad() && t.retryCount( QSmartCardData::PukType ) > 0 );
		d->changePin1InfoPinLink->setHidden( t.isSecurePinpad() );
		d->changePin2InfoPinLink->setHidden( t.isSecurePinpad() );

		if( d->dataWidget->currentIndex() == PageEmpty )
			setDataPage( t.retryCount( QSmartCardData::PukType ) == 0 ? PagePukInfo : PageCert );

		if( d->smartcard->property( "lastcard" ).toString() != t.card() &&
			t.version() == QSmartCardData::VER_3_0 )
		{
			QMessageBox box( QMessageBox::Warning, windowTitle(), tr(
				"This document is not supported for electronic use from 24.07.13, for additional information please contact "
				"<a href=\"http://www.politsei.ee/en/teenused/isikut-toendavad-dokumendid/id-kaardi-uuendamine.dot\">Police and Border Guard Board</a>."),
				QMessageBox::Ok, this );
			if( QLabel *lbl = box.findChild<QLabel*>() )
				lbl->setOpenExternalLinks( true );
			box.exec();
		}
		d->smartcard->setProperty( "lastcard", t.card() );

#ifdef Q_OS_WIN
		CertStore store;
		if( !Settings().value( "Utility/showRegCert", false ).toBool() ||
			(!store.find( t.authCert() ) || !store.find( t.signCert() )) &&
			QMessageBox::question( this, tr( "Certificate store" ),
				tr( "Certificate is not registered in certificate store. Register now?" ),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes )
		{
			QString personalCode = t.authCert().subjectInfo( "serialNumber" );
			for( const SslCertificate &c: store.list())
			{
				if( c.subjectInfo( "serialNumber" ) == personalCode )
					store.remove( c );
			}
			store.add( t.authCert(), t.card() );
			store.add( t.signCert(), t.card() );
		}
#endif
	}
	else
	{
		d->certUpdate->setProperty("updateEnabled", false);

		const QList<QLabel*> list({ d->personalName, d->surName, d->personalCode, d->personalBirth, d->personalCitizen, d->personalEmail });
		for( QLabel *l: list )
		{
			l->clear();
			l->setToolTip( QString() );
		}

		d->pictureFrame->setProperty( "PICTURE", QVariant() );
		d->pictureFrame->clear();
		setDataPage( PageEmpty );

		QString text;
		QString additional;
		if( !QPCSC::instance().serviceRunning() )
			text = tr("PCSC service is not running");
		else if( t.readers().isEmpty() )
			text = tr("No reader found");
		else if( t.cards().isEmpty() )
		{
			text = tr("No card found");
			additional = tr("Check if the ID-card is inserted correctly to the reader.<br />New ID-cards have chip on the back side of the card.");
		}
		else
			text = tr( "Reading data" );
		d->cardInfo->setAlignment( Qt::AlignCenter );
		text = "<font style='font-size:18px; font-weight: bold; color:#ec2b2b;'>" + text + "</font>";
		if( !additional.isEmpty() )
			text += "<br /><font style='font-size:12px; color:#ec2b2b;'>" + additional + "</font>";
		d->cardInfo->setText( text );
	}
	Common::setAccessibleName( d->cardInfo );
	d->loadPicture->setVisible( !t.authCert().isNull() && d->pictureFrame->property("PICTURE").isNull() );
	d->savePicture->setHidden(d->pictureFrame->property("PICTURE").isNull() ||
		Settings(QSettings::SystemScope).value("disableSave", false).toBool());

	d->cards->clear();
	d->cards->addItems( t.cards() );
	d->cards->setVisible( t.cards().size() > 1 );
	d->cards->setCurrentIndex( d->cards->findText( t.card() ) );
}
