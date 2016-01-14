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

#include "QSmartCard_p.h"

#include <common/Common.h>
#include <common/IKValidator.h>
#include <common/PinDialog.h>
#include <common/QPCSC.h>
#include <common/Settings.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

#include <openssl/evp.h>

QSmartCardData::QSmartCardData(): d( new QSmartCardDataPrivate ) {}
QSmartCardData::QSmartCardData( const QSmartCardData &other ): d( other.d ) {}
QSmartCardData::~QSmartCardData() {}
QSmartCardData& QSmartCardData::operator =( const QSmartCardData &other ) { d = other.d; return *this; }

QString QSmartCardData::card() const { return d->card; }
QStringList QSmartCardData::cards() const { return d->cards; }

bool QSmartCardData::isNull() const
{ return d->data.isEmpty() && d->authCert.isNull() && d->signCert.isNull(); }
bool QSmartCardData::isPinpad() const { return d->pinpad; }
bool QSmartCardData::isSecurePinpad() const
{ return d->reader.contains( "EZIO SHIELD", Qt::CaseInsensitive ); }
bool QSmartCardData::isValid() const
{ return d->data.value( Expiry ).toDateTime() >= QDateTime::currentDateTime(); }

QString QSmartCardData::reader() const { return d->reader; }
QStringList QSmartCardData::readers() const { return d->readers; }

QVariant QSmartCardData::data( PersonalDataType type ) const
{ return d->data.value( type ); }
SslCertificate QSmartCardData::authCert() const { return d->authCert; }
SslCertificate QSmartCardData::signCert() const { return d->signCert; }
quint8 QSmartCardData::retryCount( PinType type ) const { return d->retry.value( type ); }
ulong QSmartCardData::usageCount( PinType type ) const { return d->usage.value( type ); }
QSmartCardData::CardVersion QSmartCardData::version() const { return d->version; }

QString QSmartCardData::typeString( QSmartCardData::PinType type )
{
	switch( type )
	{
	case Pin1Type: return "PIN1";
	case Pin2Type: return "PIN2";
	case PukType: return "PUK";
	}
	return "";
}



int QSmartCardPrivate::rsa_sign( int type, const unsigned char *m, unsigned int m_len,
		unsigned char *sigret, unsigned int *siglen, const RSA *rsa )
{
	if( type != NID_md5_sha1 && m_len != 36 )
		return 0;

	QSmartCardPrivate *d = (QSmartCardPrivate*)RSA_get_app_data( rsa );
	if ( !d )
		return 0;

	try
	{
		ByteVec vec = d->card->sign( ByteVec( m, m + m_len ), EstEidCard::SSL, EstEidCard::AUTH );
		if( vec.size() == 0 )
			return 0;
		*siglen = (unsigned int)vec.size();
		memcpy( sigret, &vec[0], vec.size() );
		return 1;
	}
	catch( const std::runtime_error &e )
	{
		qDebug() << Q_FUNC_INFO << e.what();
	}
	return 0;
}

QSmartCard::ErrorType QSmartCardPrivate::handleAuthError( QSmartCardData::PinType type, const AuthError &e )
{
	updateCounters( t.d );
	if( t.retryCount( type ) < 1 ) return QSmartCard::BlockedError;
	switch( (e.SW1 << 8) + e.SW2 )
	{
	case 0x63C0: return QSmartCard::BlockedError; //pin retry count 0
	case 0x63C1: // Validate error, 1 tries left
	case 0x63C2: // Validate error, 2 tries left
	case 0x63C3: return QSmartCard::ValidateError;
	case 0x6400: // Timeout (SCM)
	case 0x6401: return QSmartCard::CancelError; // Cancel (OK, SCM)
	case 0x6402: return QSmartCard::DifferentError;
	case 0x6403: return QSmartCard::LenghtError;
	case 0x6983: return QSmartCard::BlockedError;
	case 0x6985: return QSmartCard::OldNewPinSameError;
	case 0x6A80: return QSmartCard::OldNewPinSameError;
	default: return QSmartCard::UnknownError;
	}
}

void QSmartCardPrivate::updateCounters( QSmartCardDataPrivate *d )
{
	try
	{
		d->retry.clear();
		d->usage.clear();
		card->getRetryCounts( d->retry[QSmartCardData::PukType], d->retry[QSmartCardData::Pin1Type], d->retry[QSmartCardData::Pin2Type] );
		card->getKeyUsageCounters( d->usage[QSmartCardData::Pin1Type], d->usage[QSmartCardData::Pin2Type] );
	}
	catch( const std::runtime_error &e )
	{
		qDebug() << Q_FUNC_INFO << e.what();
	}
}




QSmartCard::QSmartCard( const QString &lang, QObject *parent )
:	QThread( parent )
,	d( new QSmartCardPrivate )
{
	SCardLog::writeLog("[%s:%d] %s %s", __FUNC__, __LINE__,
		qApp->applicationName().toUtf8().constData(), qApp->applicationVersion().toUtf8().constData());
	setLang( lang );
	d->t.d->readers = QStringList() << "loading";
	d->t.d->cards = QStringList() << "loading";
	d->t.d->card = "loading";
}

QSmartCard::~QSmartCard()
{
	d->terminate = true;
	wait();
	delete d;
}

QSmartCard::ErrorType QSmartCard::change( QSmartCardData::PinType type, const QString &newpin, const QString &pin )
{
	QMutexLocker locker( &d->m );
	if( !d->card ) return QSmartCard::UnknownError;
	d->cmd = QSmartCardPrivate::Change;
	d->type = type;
	d->pin = newpin.toUtf8().constData();
	d->old = pin.toUtf8().constData();
	d->e.exec();
	return d->result;
}

QSmartCardData QSmartCard::data() const { return d->t; }

Qt::HANDLE QSmartCard::key()
{
	RSA *rsa = RSAPublicKey_dup( (RSA*)d->t.authCert().publicKey().handle() );
	if ( !rsa )
		return 0;

	RSA_set_method( rsa, &d->method );
	rsa->flags |= RSA_FLAG_SIGN_VER;
	RSA_set_app_data( rsa, d );
	EVP_PKEY *key = EVP_PKEY_new();
	EVP_PKEY_set1_RSA( key, rsa );
	RSA_free( rsa );
	return Qt::HANDLE(key);
}

QSmartCard::ErrorType QSmartCard::login( QSmartCardData::PinType type )
{
	PinDialog::PinFlags flags = PinDialog::Pin1Type;
	QSslCertificate cert;
	switch( type )
	{
	case QSmartCardData::Pin1Type: flags = PinDialog::Pin1Type; cert = d->t.authCert(); break;
	case QSmartCardData::Pin2Type: flags = PinDialog::Pin2Type; cert = d->t.signCert(); break;
	default: return UnknownError;
	}

	QScopedPointer<PinDialog> p;
	PinString pin;
	if( !d->t.isPinpad() )
	{
		p.reset( new PinDialog( flags, cert, 0, qApp->activeWindow() ) );
		if( !p->exec() )
			return CancelError;
		pin = p->text().toUtf8().constData();
	} else {
		p.reset( new PinDialog( PinDialog::PinFlags(flags|PinDialog::PinpadFlag), cert, 0, qApp->activeWindow() ) );
		p->open();
	}

	d->m.lock();
	if( !d->card ) return QSmartCard::UnknownError;
	d->cmd = QSmartCardPrivate::Validate;
	d->type = type;
	d->pin = pin;
	connect( this, SIGNAL(eventStarted()), p.data(), SIGNAL(startTimer()) );
	d->e.exec();
	if( d->result != QSmartCard::NoError )
		d->m.unlock();
	return d->result;
}

void QSmartCard::logout()
{
	d->updateCounters( d->t.d );
	d->m.unlock();
}

void QSmartCard::reload() { selectCard( d->t.card() ); }

void QSmartCard::run()
{
	const QByteArray AID35 =		QByteArray::fromHex("00A40400 0F D23300000045737445494420763335");
	const QByteArray UPDATER_AID =	QByteArray::fromHex("00A40400 0A D2330000005550443101");
	const QByteArray MASTER_FILE =	QByteArray::fromHex("00A4000C 00");
	const QByteArray ESTEIDDF =		QByteArray::fromHex("00A4010C 02 EEEE");
	const QByteArray PERSONALDATA =	QByteArray::fromHex("00A4020C 02 5044");
	const QByteArray READRECORD =	QByteArray::fromHex("00B20004 00");

	while( !d->terminate )
	{
		if( d->m.tryLock() )
		{
			// Get list of available cards
			QMap<QString,std::string> cards;
			const QStringList readers = QPCSC::instance().readers();
			for(const QString &reader: readers)
			{
				qDebug() << "Reader" << reader;
				QScopedPointer<QPCSCReader> ctx(new QPCSCReader(reader, &QPCSC::instance()));
				if(!ctx->connect(QPCSCReader::Shared))
					continue;

				if(!ctx->transfer(MASTER_FILE).resultOk())
				{
					// Master file selection failed, test if it is updater applet
					if(!ctx->transfer(UPDATER_AID).resultOk())
						continue; // Updater applet not found
					if(!ctx->transfer(MASTER_FILE).resultOk())
					{	//Found updater applet but cannot select master file, select back 3.5
						ctx->transfer(AID35);
						continue;
					}
				}

				if(!ctx->transfer(ESTEIDDF).resultOk() ||
					!ctx->transfer(PERSONALDATA).resultOk() )
					continue;
				QByteArray cardid = READRECORD;
				cardid[2] = 8;
				QString nr = d->codec->toUnicode( ctx->transfer( cardid ).data );
				if(!nr.isEmpty())
					cards[nr] = reader.toStdString();
			}

			// cardlist has changed
			QStringList order = cards.keys();
			std::sort( order.begin(), order.end(), TokenData::cardsOrder );
			bool update = d->t.cards() != order || d->t.readers() != readers;

			// check if selected card is still in slot
			if( !d->t.card().isEmpty() && !order.contains( d->t.card() ) )
			{
				update = true;
				d->card.clear();
				d->t.d = new QSmartCardDataPrivate();
			}

			d->t.d->cards = order;
			d->t.d->readers = readers;

			// if none is selected select first from cardlist
			if( d->t.card().isEmpty() && !d->t.cards().isEmpty() )
				selectCard( d->t.cards().first() );

			// read card data
			if( d->t.cards().contains( d->t.card() ) && !d->card )
			{
				update = true;
				try
				{
					QSmartCardDataPrivate *t = d->t.d;
					std::string reader = cards.value( t->card );

					d->card.reset(new EstEidCard( reader ));
					d->card->setReaderLanguageId( d->lang );

					d->updateCounters( t );
					std::vector<std::string> data;
					d->card->readPersonalData( data, EstEidCard::SURNAME, EstEidCard::COMMENT4 );
					ByteVec authcert = d->card->getAuthCert();
					ByteVec signcert = d->card->getSignCert();

					t->reader = reader.c_str();
					t->pinpad = d->card->isSecureConnection();
					t->version = QSmartCardData::CardVersion(d->card->getCardVersion());
					try {
						if( t->version > QSmartCardData::VER_1_1 )
						{
							d->card->sendApplicationIdentifierPreV3();
							t->version = QSmartCardData::VER_3_0;
						}
					} catch( const std::runtime_error &e ) {
						qDebug() << Q_FUNC_INFO << "Card is not V3.0: " << e.what();
					}
					if(t->version == QSmartCardData::VER_INVALID)
					{
						QPCSCReader *ctx = new QPCSCReader(QString::fromStdString(reader), &QPCSC::instance());
						if(ctx->connect() && ctx->transfer(QByteArray::fromHex("00A40400 0A D2330000005550443101")).resultOk())
						{
							t->version = QSmartCardData::VER_UPDATER;
							//Found updater applet, test if it is usable
							if(!ctx->transfer(MASTER_FILE).resultOk())
								ctx->transfer(AID35);
						}
						delete ctx;
					}

					t->data[QSmartCardData::SurName] =
						SslCertificate::formatName( d->encode( data[EstEidCard::SURNAME] ) ).trimmed();
					QStringList name = QStringList()
						<< SslCertificate::formatName( d->encode( data[EstEidCard::FIRSTNAME] ) ).trimmed()
						<< SslCertificate::formatName( d->encode( data[EstEidCard::MIDDLENAME] ) ).trimmed();
					name.removeAll( "" );
					t->data[QSmartCardData::FirstName] = name.join(" ");
					t->data[QSmartCardData::Sex] = d->encode( data[EstEidCard::SEX] );
					t->data[QSmartCardData::Citizen] = d->encode( data[EstEidCard::CITIZEN] );
					t->data[QSmartCardData::BirthDate] = QDate::fromString( d->encode( data[EstEidCard::BIRTHDATE] ), "dd.MM.yyyy" );
					t->data[QSmartCardData::Id] = d->encode( data[EstEidCard::ID] );
					t->data[QSmartCardData::DocumentId] = d->encode( data[EstEidCard::DOCUMENTID] );
					t->data[QSmartCardData::Expiry] = QDate::fromString( d->encode( data[EstEidCard::EXPIRY] ), "dd.MM.yyyy" );
					t->data[QSmartCardData::BirthPlace] =
						SslCertificate::formatName( d->encode( data[EstEidCard::BIRTHPLACE] ) );
					t->data[QSmartCardData::IssueDate] = QDate::fromString( d->encode( data[EstEidCard::ISSUEDATE] ), "dd.MM.yyyy" );
					t->data[QSmartCardData::ResidencePermit] = d->encode( data[EstEidCard::RESIDENCEPERMIT] );
					t->data[QSmartCardData::Comment1] = d->encode( data[EstEidCard::COMMENT1] );
					t->data[QSmartCardData::Comment2] = d->encode( data[EstEidCard::COMMENT2] );
					t->data[QSmartCardData::Comment3] = d->encode( data[EstEidCard::COMMENT3] );
					t->data[QSmartCardData::Comment4] = d->encode( data[EstEidCard::COMMENT4] );

					if( !authcert.empty() )
						t->authCert = QSslCertificate( QByteArray::fromRawData( (char*)authcert.data(), (int)authcert.size() ), QSsl::Der );
					if( !signcert.empty() )
						t->signCert = QSslCertificate( QByteArray::fromRawData( (char*)signcert.data(), (int)signcert.size() ), QSsl::Der );

					QStringList mailaddresses = t->authCert.alternateSubjectNames().values( QSsl::EmailEntry );
					t->data[QSmartCardData::Email] = !mailaddresses.isEmpty() ? mailaddresses.first() : "";

					if( t->authCert.type() & SslCertificate::DigiIDType )
					{
						t->data[QSmartCardData::FirstName] = t->authCert.toString( "GN" );
						t->data[QSmartCardData::SurName] = t->authCert.toString( "SN" );
						t->data[QSmartCardData::Id] = t->authCert.subjectInfo("serialNumber");
						t->data[QSmartCardData::BirthDate] = IKValidator::birthDate( t->authCert.subjectInfo("serialNumber") );
						t->data[QSmartCardData::IssueDate] = t->authCert.effectiveDate();
						t->data[QSmartCardData::Expiry] = t->authCert.expiryDate();
					}
				}
				catch( const std::runtime_error &e )
				{
					d->card.clear();
					qDebug() << Q_FUNC_INFO << "Error on loading card data: " << e.what();
				}
			}

			// update data if something has changed
			if( update )
				Q_EMIT dataChanged();
			d->m.unlock();
		}
		else if( d->e.isRunning() )
		{
			emit eventStarted();
			try
			{
				byte retries = 0;
				d->result = QSmartCard::UnknownError;
				switch( d->cmd )
				{
				case QSmartCardPrivate::Change:
					switch( d->type )
					{
					case QSmartCardData::Pin1Type:
						d->card->changeAuthPin( d->pin, d->old, retries );
						break;
					case QSmartCardData::Pin2Type:
						d->card->changeSignPin( d->pin, d->old, retries );
						break;
					case QSmartCardData::PukType:
						d->card->changePUK( d->pin, d->old, retries );
						break;
					}
					d->updateCounters( d->t.d );
					break;
				case QSmartCardPrivate::Unblock:
					switch( d->type )
					{
					case QSmartCardData::Pin1Type:
						d->card->unblockAuthPin( d->pin, d->old, retries );
						break;
					case QSmartCardData::Pin2Type:
						d->card->unblockSignPin( d->pin, d->old, retries );
						break;
					default: break;
					}
					d->updateCounters( d->t.d );
					break;
				case QSmartCardPrivate::Validate:
					switch( d->type )
					{
					case QSmartCardData::Pin1Type:
						d->card->validateAuthPin( d->pin, retries );
						break;
					case QSmartCardData::Pin2Type:
						d->card->validateSignPin( d->pin, retries );
						break;
					case QSmartCardData::PukType:
						d->card->validatePuk( d->pin, retries );
						break;
					default: break;
					}
					break;
				case QSmartCardPrivate::ValidateInternal:
					switch( d->type )
					{
					case QSmartCardData::Pin1Type:
						d->card->enterPin( EstEidCard::PIN_AUTH, d->pin );
						break;
					case QSmartCardData::Pin2Type:
						d->card->enterPin( EstEidCard::PIN_SIGN, d->pin );
						break;
					case QSmartCardData::PukType:
						d->card->enterPin( EstEidCard::PUK, d->pin );
						break;
					}
					break;
				default: break;
				}
				d->result = QSmartCard::NoError;
			}
			catch( const AuthError &e )
			{
				d->result = d->handleAuthError(
					d->cmd == QSmartCardPrivate::Unblock ? QSmartCardData::PukType : d->type, e );
			}
			catch( const std::runtime_error &e )
			{
				qDebug() << Q_FUNC_INFO << e.what();
			}
			d->pin.clear();
			d->old.clear();
			d->e.quit();
		}

		sleep( 5 );
	}
}

void QSmartCard::selectCard( const QString &card )
{
	d->t.d->card = card;
	d->t.d->authCert = QSslCertificate();
	d->t.d->signCert = QSslCertificate();
	d->t.d->data.clear();
	Q_EMIT dataChanged();
}

void QSmartCard::setLang( const QString &lang )
{
	d->lang = EstEidCard::EST;
	if( lang == "en" ) d->lang = EstEidCard::ENG;
	if( lang == "ru" ) d->lang = EstEidCard::RUS;
	try {
		if( d->card )
			d->card->setReaderLanguageId( d->lang );
	} catch(const PCSCManagerFailure &e ) {
		qDebug() << Q_FUNC_INFO << e.what();
	}
}

QSmartCard::ErrorType QSmartCard::unblock( QSmartCardData::PinType type, const QString &pin, const QString &puk )
{
	QMutexLocker locker( &d->m );
	if( !d->card ) return QSmartCard::UnknownError;
	d->cmd = QSmartCardPrivate::Unblock;
	d->type = type;
	d->pin = pin.toUtf8().constData();
	d->old = puk.toUtf8().constData();
	d->e.exec();
	return d->result;
}
