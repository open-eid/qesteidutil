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
 
#include "sslConnect.h"

#include <common/Common.h>
#include <common/Configuration.h>
#include <common/SOAPDocument.h>
#include <common/Settings.h>

#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QProgressDialog>

class SSLConnect::Private
{
public:
	QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
	QString errorString;
};

SSLConnect::SSLConnect(QObject *parent)
	: QNetworkAccessManager(parent)
	, d(new Private)
{
	connect(this, &QNetworkAccessManager::sslErrors, this, [=](QNetworkReply *reply, const QList<QSslError> &errors){
		QList<QSslError> ignore;
		for(const QSslError &error: errors)
		{
			switch(error.error())
			{
			case QSslError::UnableToGetLocalIssuerCertificate:
			case QSslError::CertificateUntrusted:
				//if(trusted.contains(reply->sslConfiguration().peerCertificate()))
					ignore << error;
				break;
			default: break;
			}
		}
		reply->ignoreSslErrors(ignore);
	});
}

SSLConnect::~SSLConnect()
{
	delete d;
}

QByteArray SSLConnect::getUrl(RequestType type, const QString &value)
{
	QJsonObject obj = Configuration::instance().object();
	QString label;
	QByteArray contentType;
	QNetworkRequest req;
	switch(type)
	{
	case EmailInfo:
		label = tr("Loading Email info");
		req = QNetworkRequest(
			obj.value(QLatin1String("EMAIL-REDIRECT-URL")).toString(QStringLiteral("https://sisene.www.eesti.ee/idportaal/postisysteem.naita_suunamised")));
		contentType = "application/xml";
		break;
	case ActivateEmails:
		label = tr("Loading Email info");
		req = QNetworkRequest(
			obj.value(QLatin1String("EMAIL-ACTIVATE-URL")).toString(QStringLiteral("https://sisene.www.eesti.ee/idportaal/postisysteem.lisa_suunamine?=%1")).arg(value));
		contentType = "application/xml";
		break;
	case PictureInfo:
		label = tr("Downloading picture");
		req = QNetworkRequest(
			obj.value(QLatin1String("PICTURE-URL")).toString(QStringLiteral("https://sisene.www.eesti.ee/idportaal/portaal.idpilt")));
		contentType = "image/jpeg";
		break;
	default: return QByteArray();
	}

	QProgressDialog p(label, QString(), 0, 0, qApp->activeWindow());
	p.setWindowFlags((p.windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint);
	if(QProgressBar *bar = p.findChild<QProgressBar*>())
		bar->setTextVisible( false );
	p.open();

	req.setSslConfiguration(d->ssl);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	req.setRawHeader("User-Agent", QString(QStringLiteral("%1/%2 (%3)"))
		.arg(qApp->applicationName(), qApp->applicationVersion(), Common::applicationOs()).toUtf8());
	QNetworkReply *reply = get(req);

	QEventLoop e;
	connect(reply, &QNetworkReply::finished, &e, &QEventLoop::quit);
	e.exec();

	if(reply->error() != QNetworkReply::NoError)
	{
		d->errorString = reply->errorString();
		return QByteArray();
	}
	if(!reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().contains(contentType))
	{
		d->errorString = tr("Invalid Content-Type");
		return QByteArray();
	}
	QByteArray result = reply->readAll();
	reply->deleteLater();
	return result;
}

QString SSLConnect::errorString() const { return d->errorString; }

void SSLConnect::setToken(const QSslCertificate &cert, const QSslKey &key)
{
	d->ssl.setPrivateKey(key);
	d->ssl.setLocalCertificate(cert);
}
