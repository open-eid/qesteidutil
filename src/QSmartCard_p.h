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

#include "QSmartCard.h"

#include <common/SslCertificate.h>

#include <smartcardpp/EstEIDManager.h>

#include <QtCore/QMutex>
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QtCore/QVariant>

#include <openssl/rsa.h>

typedef EstEIDManager EstEidCard;

class QSmartCardPrivate
{
public:
	enum Command
	{
		Change,
		Unblock,
		Validate,
		ValidateInternal
	};

	QSmartCardPrivate()
		: codec( QTextCodec::codecForName("Windows-1252") )
		, card(0)
		, numReaders(0)
		, terminate( false )
		, method(*RSA_get_default_method())
		, lang(EstEidCard::ENG)
		, cmd(Change)
		, type(QSmartCardData::Pin1Type)
		, result(QSmartCard::NoError)
	{
		method.name = "QSmartCard";
		method.rsa_sign = rsa_sign;
	}

	QString encode( const std::string &data ) const
	{ return data.empty() ? QString() : codec->toUnicode( QByteArray::fromRawData(data.c_str(), data.size()) ); }

	QSmartCard::ErrorType handleAuthError( QSmartCardData::PinType type, const AuthError &e );
	void updateCounters( QSmartCardDataPrivate *d );

	static int rsa_sign( int type, const unsigned char *m, unsigned int m_len,
		unsigned char *sigret, unsigned int *siglen, const RSA *rsa );

	QTextCodec		*codec;
	QSharedPointer<EstEidCard> card;
	QMutex			m;
	quint8			numReaders;
	QSmartCardData	t;
	volatile bool	terminate;
	RSA_METHOD		method;
	EstEidCard::ReaderLanguageID lang;

	// pin operations
	QEventLoop		e;
	Command			cmd;
	QSmartCardData::PinType type;
	PinString		pin, old;
	QSmartCard::ErrorType result;
};

class QSmartCardDataPrivate: public QSharedData
{
public:
	QSmartCardDataPrivate(): version(QSmartCardData::VER_INVALID), pinpad(false) {}

	QString card, reader;
	QStringList cards, readers;
	QHash<QSmartCardData::PersonalDataType,QVariant> data;
	SslCertificate authCert, signCert;
	QHash<QSmartCardData::PinType,quint8> retry;
	QHash<QSmartCardData::PinType,ulong> usage;
	QSmartCardData::CardVersion version;
	bool pinpad;
};
