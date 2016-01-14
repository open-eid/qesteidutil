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

#include "Updater.h"
#include "common/Common.h"
#include "common/Configuration.h"
#include "common/QPCSC.h"
#include "common/PinDialog.h"
#include "common/Settings.h"

#include <QtCore/QTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslKey>
#include <QtGui/QPainter>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QGridLayout>

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <thread>

#define APDU(hex) QByteArray::fromHex(hex)

class UpdaterPrivate
{
public:
	Updater *parent = nullptr;
	QPCSCReader *reader = nullptr;
	QLabel *label = nullptr;
	QPushButton *close = nullptr;
	RSA_METHOD method = *RSA_get_default_method();
	QSslCertificate cert;
	QString session;
	QNetworkRequest request;
	QByteArray signature;
	template<class T>
	QPCSCReader::Result verifyPIN(const T &cert, int p1) const;
	QtMessageHandler oldMsgHandler = nullptr;

	static int rsa_sign(int type, const unsigned char *m, unsigned int m_len,
		unsigned char *sigret, unsigned int *siglen, const RSA *rsa)
	{
		UpdaterPrivate *d = (UpdaterPrivate*)RSA_get_app_data(rsa);
		if(type != NID_md5_sha1 || m_len != 36 || !d)
			return 0;
		QEventLoop loop;
		d->signature.clear();
		Q_EMIT d->parent->signReq(&loop, QByteArray::fromRawData((const char*)m, m_len));
		if(loop.exec() != 1)
			return 0;
		*siglen = (unsigned int)d->signature.size();
		memcpy(sigret, d->signature.constData(), d->signature.size());
		return 1;
	}
};

template<class T>
QPCSCReader::Result UpdaterPrivate::verifyPIN(const T &title, int p1) const
{
	QByteArray verify = APDU("00200000 00");
	verify[3] = p1;
	int flags = (p1 == 1 ? PinDialog::Pin1Type : PinDialog::Pin2Type);
	if(reader->isPinPad())
		flags |= PinDialog::PinpadFlag;
	TokenData::TokenFlags token = 0;

	Q_FOREVER
	{
		PinDialog pin(PinDialog::PinFlags(flags), title, token, parent);
		QPCSCReader::Result result;
		if(flags & PinDialog::PinpadFlag)
		{
			std::thread([&]{
				Q_EMIT pin.startTimer();
				result = reader->transferCTL(verify, true);
				Q_EMIT pin.finish(0);
			}).detach();
			pin.exec();
		}
		else if(pin.exec() != 0)
		{
			verify[4] = pin.text().size();
			result = reader->transfer(verify + pin.text().toLatin1());
		}
		switch( (quint8(result.SW[0]) << 8) + quint8(result.SW[1]) )
		{
		case 0x63C1: token = TokenData::PinFinalTry; continue; // Validate error, 1 tries left
		case 0x63C2: token = TokenData::PinCountLow; continue; // Validate error, 2 tries left
		case 0x63C3: continue;
		case 0x63C0: // Blocked
		case 0x6400: // Timeout
		case 0x6401: // Cancel
		case 0x6402: // Mismatch
		case 0x6403: // Lenght error
		case 0x6983: // Blocked
		case 0x9000: // No error
		default: return result;
		}
	}
}

void Updater::sign(QEventLoop *loop, const QByteArray &in)
{
	if(!d->reader ||
		!d->reader->connect() ||
		!d->reader->beginTransaction())
		return loop->exit(0);

	// Verify PIN
	if(!d->verifyPIN(d->cert, 1).resultOk())
	{
		d->reader->endTransaction();
		d->reader->disconnect();
		return loop->exit(0);
	}

	// Set card parameters
	if(!d->reader->transfer(APDU("0022F301 00")).resultOk() || // SecENV 1
		!d->reader->transfer(APDU("002241B8 02 8300")).resultOk()) //Key reference, 8303801100
	{
		d->reader->endTransaction();
		d->reader->disconnect();
		return loop->exit(0);
	}

	// calc signature
	QByteArray cmd = APDU("00880000 00");
	cmd[4] = in.length();
	cmd += in;
	QPCSCReader::Result result = d->reader->transfer(cmd);
	d->reader->endTransaction();
	d->reader->disconnect();
	if(!result.resultOk())
		return loop->exit(0);

	d->signature = result.data;
	return loop->exit(1);
}



Updater::Updater(const QString &reader, QWidget *parent)
	: QDialog(parent)
	, d(new UpdaterPrivate)
{
	d->parent = this;
	setWindowTitle(parent->windowTitle());
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	connect(this, &Updater::signReq, this, &Updater::sign, Qt::QueuedConnection);

	d->reader = new QPCSCReader(reader, &QPCSC::instance());

	d->method.name = "Updater";
	d->method.rsa_sign = UpdaterPrivate::rsa_sign;

	d->label = new QLabel(this);
	d->label->setWordWrap(true);
	d->label->setMinimumHeight(30);
	QPushButton *details = new QPushButton(tr("Details"), this);
	d->close = new QPushButton(tr("Close"), this);
	d->close->hide();
	QTextBrowser *log = new QTextBrowser(this);
	log->hide();
	connect(details, &QPushButton::clicked, [=]{
		log->setVisible(!log->isVisible());
		qApp->processEvents();
		resize(width(), log->isVisible() ? 600 : 20);
	});
	connect(d->close, &QPushButton::clicked, this, &Updater::accept);
	connect(this, &Updater::log, log, &QTextBrowser::append, Qt::QueuedConnection);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(d->label, 0, 0);
	layout->addWidget(details, 0, 1);
	layout->addWidget(d->close, 0, 2);
	layout->addWidget(log, 1, 0, 1, 3);
	layout->setColumnStretch(0, 1);
	move(parent->geometry().left(), parent->geometry().center().y() - height());
	resize(parent->width(), height());

	static Updater *instance = nullptr;
	instance = this;
	d->oldMsgHandler = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &msg){
		if(!msg.contains("QObject")) //Silence Qt warnings
			Q_EMIT instance->log(msg);
	});
}

Updater::~Updater()
{
	d->reader->endTransaction();
	delete d->reader;
	qInstallMessageHandler(d->oldMsgHandler);
	delete d;
}

void Updater::process(const QByteArray &data)
{
#if QT_VERSION >= 0x050400
	qDebug().noquote() << ">" << data;
#else
	qDebug() << ">" << data;
#endif
	QJsonObject obj = QJsonDocument::fromJson(data).object();

	if(d->session.isEmpty())
		d->session = obj.value("session").toString();
	QString cmd = obj.value("cmd").toString();
	if(cmd == "CONNECT")
	{
		QPCSCReader::Mode mode = QPCSCReader::Mode(QPCSCReader::T0|QPCSCReader::T1);
		if(obj.value("protocol").toString() == "T=0") mode = QPCSCReader::T0;
		if(obj.value("protocol").toString() == "T=1") mode = QPCSCReader::T1;
		quint32 err = 0;
#ifdef Q_OS_WIN
		err = d->reader->connectEx(QPCSCReader::Exclusive, mode);
#else
		if((err = d->reader->connectEx(QPCSCReader::Exclusive, mode)) == 0 ||
			(err = d->reader->connectEx(QPCSCReader::Shared, mode)) == 0)
			d->reader->beginTransaction();
#endif
		QVariantHash ret{
			{"CONNECT", d->reader->isConnected() ? "OK" : "NOK"},
			{"reader", d->reader->name()},
			{"atr", d->reader->atr()},
			{"protocol", d->reader->protocol() == 2 ? "T=1" : "T=0"},
			{"pinpad", d->reader->isPinPad()}
		};
		if(err)
			ret["ERROR"] = QString::number(err, 16);
		Q_EMIT send(ret);
	}
	else if(cmd == "DISCONNECT")
	{
		d->reader->endTransaction();
		d->reader->disconnect([](const QString &action) {
			if(action == "leave") return QPCSCReader::LeaveCard;
			if(action == "eject") return QPCSCReader::EjectCard;
			return QPCSCReader::ResetCard;
		}(obj.value("action").toString()));
		Q_EMIT send({{"DISCONNECT", "OK"}});
	}
	else if(cmd == "APDU")
	{
		std::thread([=]{
			QPCSCReader::Result result = d->reader->transfer(APDU(obj.value("bytes").toString().toLatin1()));
			QVariantHash ret;
			ret["APDU"] = result.err ? "NOK" : "OK";
			ret["bytes"] = QByteArray(result.data + result.SW).toHex();
			if(result.err)
				ret["ERROR"] = QString::number(result.err, 16);
			Q_EMIT send(ret);
		}).detach();
	}
	else if(cmd == "MESSAGE")
	{
		d->label->setText(obj.value("text").toString());
		Q_EMIT send({{"MESSAGE", "OK"}});
	}
	else if(cmd == "DIALOG")
	{
		QMessageBox box(QMessageBox::Information, windowTitle(),
			obj.value("text").toString(), QMessageBox::Yes|QMessageBox::No, this);
		for(QLabel *l: box.findChildren<QLabel*>())
			Common::setAccessibleName(l);
		Q_EMIT send({{"DIALOG", "OK"}, {"button", box.exec() == QMessageBox::Yes ? "green" : "red"}});
	}
	else if(cmd == "VERIFY")
	{
		QPCSCReader::Result result = d->verifyPIN(obj.value("text").toString(), obj.value("p2").toInt(1));
		Q_EMIT send({
			{"VERIFY", result.resultOk() ? "OK" : "NOK"},
			{"bytes", QByteArray(result.data + result.SW).toHex()}
		});
	}
	else if(cmd == "DECRYPT")
	{
		QPCSCReader::Result result = d->reader->transfer(APDU(obj.value("bytes").toString().toLatin1()));
		if(result.resultOk())
		{
			QMessageBox box(QMessageBox::Information, windowTitle(),
				QString(), QMessageBox::Yes|QMessageBox::No, this);
			QPixmap pinEnvelope(QSize(300, 100));
			QPainter p(&pinEnvelope);
			p.fillRect(pinEnvelope.rect(), Qt::white);
			p.setPen(Qt::black);
			p.drawText(pinEnvelope.rect(), Qt::AlignCenter, QString::fromUtf8(result.data));
			for(QLabel *l: box.findChildren<QLabel*>())
			{
				if(!l->pixmap())
					l->setPixmap(pinEnvelope);
			}
			Q_EMIT send({{"DECRYPT", "OK"}, {"button", box.exec() == QMessageBox::Yes ? "green" : "red"}});
		}
		else
		{
			QVariantHash ret;
			ret["APDU"] = "NOK";
			ret["bytes"] = QByteArray(result.data + result.SW).toHex();
			if(result.err)
				ret["ERROR"] = QString::number(result.err, 16);
			Q_EMIT send(ret);
		}
	}
	else if(cmd == "STOP")
	{
		if(obj.contains("text"))
			d->label->setText(obj.value("text").toString());
		d->close->show();
	}
	else
		Q_EMIT send({{"CMD", "UNKNOWN"}});
}

int Updater::exec()
{
	// Read certificate
	d->reader->connect();
	d->reader->beginTransaction();
	if(!d->reader->transfer(APDU("00A40000 00")).resultOk())
	{
		// Master file selection failed, test if it is updater applet
		d->reader->transfer(APDU("00A40400 0A D2330000005550443101"));
		d->reader->transfer(APDU("00A40000 00"));
	}
	d->reader->transfer(APDU("00A40000 00"));
	d->reader->transfer(APDU("00A40100 02 EEEE"));
	d->reader->transfer(APDU("00A40200 02 AACE"));
	QByteArray certData;
	for(int pos = 0; pos < 0x600; pos += 256) {
		QByteArray apdu = APDU("00B00000 00");
		apdu[2] = pos >> 8;
		apdu[3] = pos;
		QPCSCReader::Result result = d->reader->transfer(apdu);
		if(!result.resultOk())
		{
			d->reader->endTransaction();
			d->label->setText(tr("Failed to read certificate"));
			return QDialog::exec();
		}
		certData += result.data;
	}
	d->reader->endTransaction();
	d->reader->disconnect();

	// Associate certificate and key with operation.
	d->cert = QSslCertificate(certData, QSsl::Der);
	EVP_PKEY *key = nullptr;
	if(!d->cert.isNull())
	{
		RSA *rsa = RSAPublicKey_dup((RSA*)d->cert.publicKey().handle());
		RSA_set_method(rsa, &d->method);
		rsa->flags |= RSA_FLAG_SIGN_VER;
		RSA_set_app_data(rsa, d);
		key = EVP_PKEY_new();
		EVP_PKEY_set1_RSA(key, rsa);
		//RSA_free(rsa);
	}

	// Do connection
	QNetworkAccessManager *net = new QNetworkAccessManager(this);
	d->request = QNetworkRequest(QUrl(
		Configuration::instance().object().value("EIDUPDATER-URL").toString()));
	d->request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	d->request.setRawHeader("User-Agent", QString("%1/%2 (%3)")
		.arg(qApp->applicationName()).arg(qApp->applicationVersion()).arg(Common::applicationOs()).toUtf8());
	qDebug() << "Connecting to" << d->request.url().toString();

	QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
	QList<QSslCertificate> trusted;
	for(const QJsonValue &cert: Configuration::instance().object().value("CERT-BUNDLE").toArray())
		trusted << QSslCertificate(QByteArray::fromBase64(cert.toString().toLatin1()), QSsl::Der);
	ssl.setCaCertificates(QList<QSslCertificate>());
	ssl.setProtocol(QSsl::TlsV1);
	if(key)
	{
		ssl.setPrivateKey(QSslKey(key));
		ssl.setLocalCertificate(d->cert);
	}
	d->request.setSslConfiguration(ssl);

	// Get proxy settings
	QNetworkProxy proxy = []() -> const QNetworkProxy {
		for(const QNetworkProxy &proxy: QNetworkProxyFactory::systemProxyForQuery())
			if(proxy.type() == QNetworkProxy::HttpProxy)
				return proxy;
		return QNetworkProxy();
	}();
	Settings s(qApp->applicationName());
	QString proxyHost = s.value("PROXY-HOST").toString();
	if(!proxyHost.isEmpty())
	{
		proxy.setHostName(proxyHost.split(':').at(0));
		proxy.setPort(proxyHost.split(':').at(1).toUInt());
	}
	proxy.setUser(s.value("PROXY-USER", proxy.user()).toString());
	proxy.setPassword(s.value("PROXY-PASS", proxy.password()).toString());
	proxy.setType(QNetworkProxy::HttpProxy);
	net->setProxy(proxy.hostName().isEmpty() ? QNetworkProxy() : proxy);
	qDebug() << "Proxy" << proxy.hostName() << ":" << proxy.port() << "User" << proxy.user();

	connect(net, &QNetworkAccessManager::sslErrors, this, [=](QNetworkReply *reply, const QList<QSslError> &errors){
		QList<QSslError> ignore;
		for(const QSslError &error: errors)
		{
			switch(error.error())
			{
			case QSslError::UnableToGetLocalIssuerCertificate:
			case QSslError::CertificateUntrusted:
				if(trusted.contains(reply->sslConfiguration().peerCertificate()))
					ignore << error;
				break;
			default: break;
			}
		}
		reply->ignoreSslErrors(ignore);
	});
	connect(this, &Updater::send, net, [=](const QVariantHash &response){
		QJsonObject resp;
		if(!d->session.isEmpty())
			resp["session"] = d->session;
		for(QVariantHash::const_iterator i = response.constBegin(); i != response.constEnd(); ++i)
			resp[i.key()] = QJsonValue::fromVariant(i.value());
		QByteArray data = QJsonDocument(resp).toJson(QJsonDocument::Compact);
#if QT_VERSION >= 0x050400
		qDebug().noquote() << "<" << data;
#else
		qDebug() << "<" << data;
#endif
		QNetworkReply *reply = net->post(d->request, data);
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(true);
		connect(timer, &QTimer::timeout, reply, [=]{
			d->label->setText(tr("Request timed out"));
			d->close->show();
		});
		connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
		timer->start(30*1000);
	}, Qt::QueuedConnection);
	connect(net, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply){
		switch(reply->error())
		{
		case QNetworkReply::NoError:
			if(reply->header(QNetworkRequest::ContentTypeHeader) == "application/json")
			{
				QByteArray data = reply->readAll();
				delete reply;
				process(data);
				return;
			}
			else
			{
				d->label->setText("<b><font color=\"red\">" + tr("Invalid content type") + "</font></b>");
				d->close->show();
			}
			break;
		default:
			switch(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
			{
			case 503:
			case 509:
				d->label->setText("<b>" + tr("Updating certificates has failed. The server is overloaded, try again later.") + "</b>");
				break;
			default:
				d->label->setText("<b><font color=\"red\">" + reply->errorString() + "</font></b>");
			}
			d->close->show();
		}
		reply->deleteLater();
	}, Qt::QueuedConnection);

	QComboBox *lang = parent()->findChild<QComboBox*>("languages");
	Q_EMIT send({
		{"cmd", "START"},
		{"lang", lang ? lang->currentData().toString() : "et"},
		{"platform", qApp->applicationOs()},
		{"version", qApp->applicationVersion()}
	});
	return QDialog::exec();
}
