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

JsCertData::JsCertData( QObject *parent ): QObject( parent ) {}

QSslCertificate JsCertData::cert() const { return m_qcert; }

void JsCertData::loadCert(EstEidCard *card, CertType ct)
{
	if ( !card )
		return;

	certBytes.clear();

    try {
        // Read certificate
		certBytes = ct == AuthCert ? card->getAuthCert() : card->getSignCert();
		if( certBytes.size() )
			m_qcert = QSslCertificate(QByteArray((char *)&certBytes[0], certBytes.size()), QSsl::Der);
		else
			m_qcert = QSslCertificate();
	} catch ( std::runtime_error &err ) {
//        doShowError(err);
		m_qcert = QSslCertificate();
		qDebug() << "Error on loadCert: " << err.what();
    }
}

QString JsCertData::getEmail()
{
	if ( isTempel() )
		return m_qcert.subjectInfo( "emailAddress" );

	QStringList mailaddresses = m_qcert.alternateSubjectNames().values(QSsl::EmailEntry);

    // return first email address
    if (!mailaddresses.isEmpty())
        return mailaddresses.first();
    else
        return "";
}

QString JsCertData::getSerialNum()
{ return m_qcert.subjectInfo("serialNumber"); }

QString JsCertData::getString( const QString &str )
{ return SslCertificate( m_qcert ).toString( str ); }

QString JsCertData::getSubjCN()
{ return SslCertificate( m_qcert ).subjectInfo(QSslCertificate::CommonName); }

QString JsCertData::getSubjSN()
{ return m_qcert.subjectInfo("SN"); }

QString JsCertData::getSubjO()
{ return m_qcert.subjectInfo(QSslCertificate::Organization); }

QString JsCertData::getSubjOU()
{ return m_qcert.subjectInfo(QSslCertificate::OrganizationalUnitName); }

QString JsCertData::getValidFrom( const QString &locale )
{ return QLocale( locale ).toString( m_qcert.effectiveDate().toLocalTime(), "dd. MMMM yyyy" ); }

QString JsCertData::getValidTo( const QString &locale )
{ return QLocale( locale ).toString( m_qcert.expiryDate().toLocalTime(), "dd. MMMM yyyy" ); }

QString JsCertData::getIssuerCN()
{ return m_qcert.issuerInfo(QSslCertificate::CommonName); }

QString JsCertData::getIssuerO()
{ return m_qcert.issuerInfo(QSslCertificate::Organization); }

QString JsCertData::getIssuerOU()
{ return m_qcert.issuerInfo(QSslCertificate::OrganizationalUnitName); }

bool JsCertData::isDigiID()
{ return SslCertificate( m_qcert ).isDigiID(); }

bool JsCertData::isTempel()
{ return SslCertificate( m_qcert ).isTempel(); }

bool JsCertData::isTest()
{ return SslCertificate( m_qcert ).isTest(); }

bool JsCertData::isValid()
{ return m_qcert.expiryDate().toLocalTime() >= QDateTime::currentDateTime(); }

int JsCertData::validDays()
{ return qMax( 0, QDateTime::currentDateTime().daysTo( m_qcert.expiryDate().toLocalTime() ) ); }
