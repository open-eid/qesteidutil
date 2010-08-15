/*
 * QEstEidUtil
 *
 * Copyright (C) 2009,2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009,2010 Raul Metsma <raul@innovaatik.ee>
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

#include <QDebug>
#include <QDateTime>
#include <QLocale>
#include <QStringList>

#include "jscertdata.h"
#include "common/SslCertificate.h"

JsCertData::JsCertData( QObject *parent )
:	QObject( parent )
{
    m_card = NULL;
	m_qcert = NULL;
}

JsCertData::~JsCertData()
{
	if( m_qcert )
		delete m_qcert;
}

QSslCertificate JsCertData::cert() const { return *m_qcert; }

void JsCertData::loadCert(EstEidCard *card, CertType ct)
{
    m_card = card;

    if (!m_card) {
        qDebug("No card");
        return;
    }

	if( m_qcert )
	{
		delete m_qcert;
		m_qcert = 0;
	}

    std::vector<std::string> tmp;
    ByteVec certBytes;
    try {
        // Read certificate
        if (ct == AuthCert)
            certBytes = m_card->getAuthCert();
        else
            certBytes = m_card->getSignCert();

		if( certBytes.size() )
			m_qcert = new QSslCertificate(QByteArray((char *)&certBytes[0], certBytes.size()), QSsl::Der);
		else
			m_qcert = new QSslCertificate();
	} catch ( std::runtime_error &err ) {
//        doShowError(err);
		qDebug() << "Error on loadCert: " << err.what();
    }
}

QString JsCertData::toPem()
{
    if (!m_qcert)
        return "";

    return QString(m_qcert->toPem());
}

QString JsCertData::getEmail()
{
    if (!m_qcert)
        return "";

	if ( isTempel() )
		return m_qcert->subjectInfo( "emailAddress" );

    QStringList mailaddresses = m_qcert->alternateSubjectNames().values(QSsl::EmailEntry);

    // return first email address
    if (!mailaddresses.isEmpty())
        return mailaddresses.first();
    else
        return "";
}

QString JsCertData::getSerialNum()
{
    if (!m_qcert)
        return "";

    return m_qcert->subjectInfo("serialNumber");
}

QString JsCertData::getSubjCN()
{
    if (!m_qcert)
        return "";

	return SslCertificate( *m_qcert ).subjectInfo(QSslCertificate::CommonName);
}

QString JsCertData::getSubjSN()
{
    if (!m_qcert)
        return "";

    return m_qcert->subjectInfo("SN");
}

QString JsCertData::getSubjO()
{
    if (!m_qcert)
        return "";

    return m_qcert->subjectInfo(QSslCertificate::Organization);
}

QString JsCertData::getSubjOU()
{
    if (!m_qcert)
        return "";

    return m_qcert->subjectInfo(QSslCertificate::OrganizationalUnitName);
}

QString JsCertData::getValidFrom( const QString &locale )
{
    if (!m_qcert)
        return "";

	return QLocale( locale ).toString( m_qcert->effectiveDate().toLocalTime(), "dd. MMMM yyyy" );
}

QString JsCertData::getValidTo( const QString &locale )
{
    if (!m_qcert)
        return "";

	return QLocale( locale ).toString( m_qcert->expiryDate().toLocalTime(), "dd. MMMM yyyy" );
}

QString JsCertData::getIssuerCN()
{
    if (!m_qcert)
        return "";

    return m_qcert->issuerInfo(QSslCertificate::CommonName);
}

QString JsCertData::getIssuerO()
{
    if (!m_qcert)
        return "";

    return m_qcert->issuerInfo(QSslCertificate::Organization);
}

QString JsCertData::getIssuerOU()
{
    if (!m_qcert)
        return "";

    return m_qcert->issuerInfo(QSslCertificate::OrganizationalUnitName);
}

bool JsCertData::isTempel()
{
	if (!m_qcert)
		return false;

	return SslCertificate( *m_qcert ).isTempel();
}

bool JsCertData::isTest()
{
	if (!m_qcert)
		return false;

	return SslCertificate( *m_qcert ).isTest();
}

bool JsCertData::isValid()
{
    if (!m_qcert)
        return false;

	return m_qcert->expiryDate().toLocalTime() >= QDateTime::currentDateTime();
}

int JsCertData::validDays()
{
	if ( !m_qcert || !isValid() )
		return 0;
	
	return QDateTime::currentDateTime().daysTo( m_qcert->expiryDate().toLocalTime() );
}
