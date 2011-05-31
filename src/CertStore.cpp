/*
 * QEstEidUtil
 *
 * Copyright (C) 2011 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2011 Raul Metsma <raul@innovaatik.ee>
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

#include "CertStore.h"

#include <common/SslCertificate.h>

#include <Windows.h>
#include <WinCrypt.h>

class CertStorePrivate
{
public:
	CertStorePrivate(): s(0) {}

	PCCERT_CONTEXT certContext( const QSslCertificate &cert ) const
	{
		QByteArray data = cert.toDer();
		return CertCreateCertificateContext( X509_ASN_ENCODING,
			(const BYTE*)data.constData(), data.size() );
	}

	HCERTSTORE s;
};

CertStore::CertStore()
: d( new CertStorePrivate )
{
	d->s = CertOpenStore( CERT_STORE_PROV_SYSTEM_W,
		X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY" );
}

CertStore::~CertStore()
{
	CertCloseStore( d->s, 0 );
	delete d;
}

bool CertStore::add( const QSslCertificate &cert, const QString &card )
{
	if( !d->s )
		return false;

	SslCertificate c( cert );
	DWORD keyCode = c.keyUsage().contains( SslCertificate::NonRepudiation ) ? AT_SIGNATURE : AT_KEYEXCHANGE;

	PCCERT_CONTEXT context = d->certContext( cert );

	QString str = QString( "%1 %2" )
		.arg( keyCode == AT_SIGNATURE ? "Signature" : "Authentication" )
		.arg( c.toString( "SN, GN" ).toUpper() );
	CRYPT_DATA_BLOB DataBlob = { str.length() * sizeof( QChar ), (BYTE*)str.utf16() };
	CertSetCertificateContextProperty( context, CERT_FRIENDLY_NAME_PROP_ID, 0, &DataBlob );

	QString cardStr = QString( keyCode == AT_SIGNATURE ? "SIG_" : "AUT_" ).append( card );
	CRYPT_KEY_PROV_INFO KeyProvInfo =
	{ (LPWSTR)cardStr.utf16(), L"EstEID Card CSP", PROV_RSA_FULL, 0, 0, 0, keyCode };
	CertSetCertificateContextProperty( context, CERT_KEY_PROV_INFO_PROP_ID, 0, &KeyProvInfo );

	bool result = CertAddCertificateContextToStore( d->s, context, CERT_STORE_ADD_REPLACE_EXISTING, 0 );
	CertFreeCertificateContext( context );
	return result;
}

bool CertStore::find( const QSslCertificate &cert )
{
	if( !d->s )
		return false;

	PCCERT_CONTEXT context = d->certContext( cert );
	bool result = CertFindCertificateInStore( d->s, X509_ASN_ENCODING,
		0, CERT_FIND_SUBJECT_CERT, context->pCertInfo, 0 );
	CertFreeCertificateContext( context );
	return result;
}

void CertStore::remove( const QSslCertificate &cert )
{
	if( !d->s )
		return;

	PCCERT_CONTEXT context = d->certContext( cert );
	PCCERT_CONTEXT scontext = 0;
	while( (scontext = CertEnumCertificatesInStore( d->s, scontext )) )
	{
		if( CertCompareCertificateName( X509_ASN_ENCODING,
				&context->pCertInfo->Subject, &scontext->pCertInfo->Subject ) )
			CertDeleteCertificateFromStore( CertDuplicateCertificateContext( scontext ) );
	}
	CertFreeCertificateContext( context );
}
