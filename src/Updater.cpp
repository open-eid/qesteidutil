#include "Updater.h"
#include "common/Common.h"
#include "common/Configuration.h"
#include "common/QPCSC.h"
#include "common/PinDialog.h"
#include "common/Settings.h"

#include <QApplication>
#include <QComboBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QPainter>
#include <QPushButton>
#include <QSslKey>
#include <QSslSocket>
#include <QtEndian>
#include <QTextBrowser>
#include <QGridLayout>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>

#include <thread>

#define APDU QByteArray::fromHex
#if QT_VERSION >= 0x050400
#define DEBUG qDebug().noquote()
#else
#define DEBUG qDebug()
#endif

class UpdaterPrivate
{
public:
	QPCSCReader *reader = nullptr;
	QLabel *label = nullptr;
	BIO *bio = nullptr;
	SSL *ssl = SSL_new(SSL_CTX_new(TLSv1_client_method()));
	RSA_METHOD method = *RSA_get_default_method();
	QSslCertificate cert;
	RSA *key = nullptr;
	QString session;
	bool isRunning = true;

	static int rsa_sign(int type, const unsigned char *m, unsigned int m_len,
		unsigned char *sigret, unsigned int *siglen, const RSA *rsa);
};

int UpdaterPrivate::rsa_sign(int type, const unsigned char *m, unsigned int m_len,
	unsigned char *sigret, unsigned int *siglen, const RSA *rsa)
{
	UpdaterPrivate *d = (UpdaterPrivate*)RSA_get_app_data(rsa);
	if(type != NID_md5_sha1 ||
		m_len != 36 ||
		!d ||
		!d->reader ||
		!d->reader->connect() ||
		!d->reader->beginTransaction())
		return 0;

	// Verify PIN
	QByteArray verify = APDU("00200001 00");
	if(d->reader->isPinPad())
	{
		PinDialog pin(PinDialog::Pin1PinpadType, d->cert, 0, 0);
		Q_EMIT pin.startTimer();
		std::thread([&]{
			pin.done(d->reader->transferCTL(verify, true).resultOk() ? 1 : 0);
		}).detach();
		if(pin.exec() == 0)
		{
			d->reader->endTransaction();
			d->reader->disconnect();
			return 0;
		}
	}
	else
	{
		PinDialog pin(PinDialog::Pin1Type, d->cert, 0, 0);
		if(pin.exec() == 0)
			return 0;
		verify[4] = pin.text().size();
		QPCSCReader::Result result = d->reader->transfer(verify + pin.text().toLatin1());
		if(!result.resultOk())
		{
			d->reader->endTransaction();
			d->reader->disconnect();
			return 0;
		}
	}

	// TODO: handle invalid pin

	// Set card parameters
	if(!d->reader->transfer(APDU("0022F301 00")).resultOk() || // SecENV 1
		!d->reader->transfer(APDU("002241B8 02 8300")).resultOk()) //Key reference, 8303801100
	{
		d->reader->endTransaction();
		d->reader->disconnect();
		return 0;
	}

	// calc signature
	QByteArray cmd = APDU("00880000 00");
	cmd[4] = m_len;
	cmd += QByteArray::fromRawData((const char*)m, m_len);
	QPCSCReader::Result result = d->reader->transfer(cmd);
	d->reader->endTransaction();
	d->reader->disconnect();
	if(!result.resultOk())
		return 0;

	*siglen = (unsigned int)result.data.size();
	memcpy(sigret, result.data.constData(), result.data.size());
	return 1;
}



Updater::Updater(const QString &reader, QWidget *parent)
	: QDialog(parent)
	, d(new UpdaterPrivate)
{
	resize(600, 20);

	QPCSC *pcsc = new QPCSC(QPCSC::APDULog, this);
	d->reader = new QPCSCReader(reader, pcsc);

	SSL_CTX_set_mode(d->ssl->ctx, SSL_MODE_AUTO_RETRY);
	SSL_CTX_set_quiet_shutdown(d->ssl->ctx, 1);

	d->method.name = "Updater";
	d->method.rsa_sign = UpdaterPrivate::rsa_sign;

	d->label = new QLabel(this);
	QPushButton *details = new QPushButton(tr("Details"), this);
	QTextBrowser *log = new QTextBrowser(this);
	log->hide();
	connect(details, &QPushButton::clicked, [=]{
		log->setVisible(!log->isVisible());
		qApp->processEvents();
		resize(600, log->isVisible() ? 600 : 20);
	});
	connect(this, &Updater::log, log, &QTextBrowser::append, Qt::QueuedConnection);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(d->label, 0, 0);
	layout->addWidget(details, 0, 1);
	layout->addWidget(log, 1, 0, 1, 2);
	layout->setColumnStretch(0, 1);

	static Updater *instance = nullptr;
	instance = this;
	qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &msg){
		Q_EMIT instance->log(msg);
	});

	connect(this, &Updater::handle, this, &Updater::process, Qt::QueuedConnection);
}

Updater::~Updater()
{
	if(d->bio && !SSL_get_rbio(d->ssl))
		BIO_free(d->bio);
	SSL_CTX_free(d->ssl->ctx);
	SSL_free(d->ssl);
	if(d->key)
		RSA_free(d->key);
	d->reader->endTransaction();
	qInstallMessageHandler(nullptr);
	delete d;
}

void Updater::process(const QByteArray &data)
{
	DEBUG << ">" << data;
	QJsonObject obj = QJsonDocument::fromJson(data).object();

	if(d->session.isEmpty())
		d->session = obj.value("session").toString();
	QString cmd = obj.value("cmd").toString();
	if(cmd == "CONNECT")
	{
		DEBUG << "CONNECT";
		QPCSCReader::Mode mode = QPCSCReader::Mode(QPCSCReader::T0|QPCSCReader::T1);
		if(obj.value("protocol").toString() == "T=0") mode = QPCSCReader::T0;
		if(obj.value("protocol").toString() == "T=1") mode = QPCSCReader::T1;
		if(d->reader->connect(QPCSCReader::Shared, mode))
			d->reader->beginTransaction();
		Q_EMIT send({
			{"CONNECT", d->reader->isConnected() ? "OK" : "NOK"},
			{"reader", d->reader->name()},
			{"atr", d->reader->atr()},
			{"protocol", d->reader->protocol() == 2 ? "T=1" : "T=0"},
			{"pinpad", d->reader->isPinPad()}
		});
	}
	else if(cmd == "DISCONNECT")
	{
		DEBUG << "DISCONNECT";
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
				ret["ERROR"] = result.err;
			Q_EMIT send(ret);
		}).detach();
	}
	else if(cmd == "MESSAGE")
	{
		d->label->setText(obj.value("text").toString());
		DEBUG << "MESSAGE" << d->label->text();
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
		QByteArray verify = APDU("0020000000");
		verify[3] = obj.value("p2").toInt(1);
		if(d->reader->isPinPad())
		{
			PinDialog pin(PinDialog::Pin1PinpadType, obj.value("text").toString(), 0, this);
			Q_EMIT pin.startTimer();
			std::thread([&]{
				QPCSCReader::Result result = d->reader->transferCTL(verify, true);
				Q_EMIT send({
					{"VERIFY", result.resultOk() ? "OK" : "NOK"},
					{"bytes", QByteArray(result.data + result.SW).toHex()}
				});
				pin.done(0);
			}).detach();
			pin.exec();
		}
		else
		{
			PinDialog pin(PinDialog::Pin1Type, obj.value("text").toString(), 0, this);
			if(pin.exec() != 0)
			{
				verify[4] = pin.text().size();
				QPCSCReader::Result result = d->reader->transfer(verify + pin.text().toLatin1());
				Q_EMIT send({
					{"VERIFY", result.resultOk() ? "OK" : "NOK"},
					{"bytes", QByteArray(result.data + result.SW).toHex()}
				});
			}
			else
				Q_EMIT send({{"VERIFY", "NOK"}});
		}
	}
	else if(cmd == "DECRYPT")
	{
		QByteArray data = APDU(obj.value("payload").toString().toLatin1());
		if(!d->reader->transfer(APDU("00A40000 00")).resultOk() ||
			!d->reader->transfer(APDU("00A40100 02 EEEE")).resultOk() ||
			!d->reader->transfer(APDU("0022F306 00")).resultOk() ||
			!d->reader->transfer(APDU("002241B8 02 8300")).resultOk() ||
			!d->reader->transfer(APDU("102A8086 FF 00") + data.left(data.length() - 2)).resultOk())
		{
			Q_EMIT send({{"DECRYPT", "NOK"}});
			return;
		}

		QPCSCReader::Result result = d->reader->transfer(APDU("002A8086 02") + data.right(2));
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
			Q_EMIT send({{"DECRYPT", box.exec() == QMessageBox::Yes ? "OK" : "NOK"}});
		}
		else
			Q_EMIT send({{"DECRYPT", "NOK"}});
	}
	else if(cmd == "STOP")
	{
		d->isRunning = false;
		Q_EMIT send({{"STOP", "OK"}});
	}
	else
		Q_EMIT send({{"CMD", "UNKNOWN"}});
}

int Updater::exec()
{
	// Read certificate
	d->reader->connect();
	d->reader->beginTransaction();
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

	d->key = RSAPublicKey_dup((RSA*)d->cert.publicKey().handle());
	RSA_set_method(d->key, &d->method);
	d->key->flags |= RSA_FLAG_SIGN_VER;
	RSA_set_app_data(d->key, d);

	// Get proxy settings
	QByteArray proxyHost, proxyUser, proxyPass;
	for(const QNetworkProxy &proxy: QNetworkProxyFactory::systemProxyForQuery())
	{
		if(proxy.type() != QNetworkProxy::HttpProxy)
			continue;
		proxyHost = QString("%1:%2").arg(proxy.hostName()).arg(proxy.port()).toLatin1();
		proxyUser = proxy.user().toLatin1();
		proxyPass = proxy.password().toLatin1();
		break;
	}
	proxyHost = Settings(qApp->applicationName()).value("PROXY-HOST", proxyHost).toByteArray();
	proxyUser = Settings(qApp->applicationName()).value("PROXY-USER", proxyUser).toByteArray();
	proxyPass = Settings(qApp->applicationName()).value("PROXY-PASS", proxyPass).toByteArray();

	// Do connection
	QUrl url(Configuration::instance().object().value("EIDUPDATER-URL").toString());
	QByteArray host = QString("%1:%2").arg(url.host()).arg(url.port(443)).toLatin1();
	QByteArray header = QString("POST %1 HTTP/1.1\r\nHost: %2\r\nContent-Type: application/json\r\nContent-Length: ")
		.arg(url.path()).arg(host.constData()).toLatin1();

	if(!SSL_use_certificate(d->ssl, (X509*)d->cert.handle()) ||
		!SSL_use_RSAPrivateKey(d->ssl, d->key) ||
		!SSL_check_private_key(d->ssl) ||
		!SSL_set_tlsext_host_name(d->ssl, url.host().toLatin1().data()))
	{
		d->label->setText(tr("Failed to connect host"));
		return QDialog::exec();
	}

	d->bio = BIO_new_connect(const_cast<char*>(proxyHost.isEmpty() ? host.constData() : proxyHost.constData()));

	// Do proxy
	if(!proxyHost.isEmpty())
	{
		if(BIO_do_connect(d->bio) != 1)
		{
			d->label->setText(tr("Failed to connect host"));
			return QDialog::exec();
		}
		BIO_printf(d->bio, "CONNECT %s HTTP/1.0\r\n", host.constData());
		BIO_printf(d->bio, "Host: %s\r\n", host.constData());
		if(!proxyUser.isEmpty())
			BIO_printf(d->bio, "Proxy-Authorization: Basic %s\r\n", (proxyUser + ":" + proxyPass).toBase64().constData());
		BIO_printf(d->bio, "\r\n");
		QByteArray line(1024, 0);
		int res = BIO_read(d->bio, line.data(), line.size());
		line.resize(std::max(res, 0));
		if(res <= 0 || !line.contains("200") || !line.contains("established") || line.right(4) != "\r\n\r\n")
		{
			d->label->setText(tr("Failed to connect proxy"));
			return QDialog::exec();
		}
	}

	// Do SSL
	SSL_set_bio(d->ssl, d->bio, d->bio);
	if(SSL_connect(d->ssl) != 1)
	{
		d->label->setText(tr("Failed to connect host"));
		return QDialog::exec();
	}

	// Processing thread
	std::thread([&]{
		QEventLoop e;
		quint32 lenght = 0;
		QByteArray data(1024, 0);
		char *p = data.data();

		connect(this, &Updater::send, [&](const QVariantHash &response){
			QJsonObject resp;
			resp["session"] = d->session;
			for(QVariantHash::const_iterator i = response.constBegin(); i != response.constEnd(); ++i)
				resp[i.key()] = QJsonValue::fromVariant(i.value());
			data = QJsonDocument(resp).toJson(QJsonDocument::Compact);
			DEBUG << "<" << data;
			e.quit();
		});

		QComboBox *lang = parent()->findChild<QComboBox*>("languages");
		QJsonObject req;
		req["cmd"] = QString("START");
		req["lang"] = lang ? lang->currentData().toString() : "et";
		req["platform"] = qApp->applicationOs();
		QByteArray request = QJsonDocument(req).toJson(QJsonDocument::Compact);
		DEBUG << "<" << request;
		request.prepend(header + QByteArray::number(request.size()) + "\r\n\r\n");
		int size = SSL_write(d->ssl, request.constData(), request.size());

		while(d->isRunning) {
			size = SSL_read(d->ssl, p, data.constEnd() - p);
			if(size <= 0)
			{
				switch(SSL_get_error(d->ssl, size))
				{
				case SSL_ERROR_NONE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
					continue;
				case SSL_ERROR_ZERO_RETURN: // Close
				default: // Error
					d->isRunning = false;
					continue;
				}
			}

			if(lenght == 0)
			{
				int pos = data.indexOf("\r\n\r\n");
				if(pos == -1)
					continue;

				const QList<QByteArray> headers = data.mid(0, pos).split('\n');
				if(!headers.value(0).contains("200"))
				{
					DEBUG << "Invalid HTTP result";
					DEBUG << headers;
					return;
				}

				for(const QByteArray &line : headers)
				{
					if(line.startsWith("Content-length: ") || line.startsWith("Content-Length: "))
						lenght = line.mid(16).trimmed().toUInt();
				}
				if(lenght == 0)
				{
					DEBUG << "Failed to Content-Length header";
					DEBUG << headers;
					return;
				}

				data.remove(0, pos + 4);
				data.resize(lenght);
				p = data.data();
				size -= (pos + 4);
			}

			if((p += size) - data.constBegin() != data.size())
				continue;

			Q_EMIT handle(data);
			e.exec();
			if(!d->isRunning)
				return;

			data = data.prepend(header + QByteArray::number(data.size()) + "\r\n\r\n");
			size = SSL_write(d->ssl, data.constData(), data.size());

			data = QByteArray(1024, 0);
			lenght = 0;
			p = data.data();
		}
	}).detach();

	return QDialog::exec();
}
