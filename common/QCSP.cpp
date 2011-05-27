/*
 * QDigiDocCommon
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

#include "QCSP.h"

#include "PinDialog.h"

#include <QApplication.h>

#include <Windows.h>
#include <WinCrypt.h>

#include <openssl/obj_mac.h>

class QCSPPrivate
{
public:
	QCSPPrivate(): h(0) {}

	static QByteArray provParam( HCRYPTPROV h, DWORD param, DWORD flags = 0 );
	static QByteArray keyParam( HCRYPTKEY key, DWORD param, DWORD flags = 0 );

	HCRYPTPROV h;
};



QByteArray QCSPPrivate::keyParam( HCRYPTKEY key, DWORD param, DWORD flags )
{
	DWORD size = 0;
	if( !CryptGetKeyParam( key, param, 0, &size, flags ) )
		return QByteArray();
	QByteArray data;
	data.resize( size );
	if( !CryptGetKeyParam( key, param, (BYTE*)data.data(), &size, flags ) )
		return QByteArray();
	return data;
}

QByteArray QCSPPrivate::provParam( HCRYPTPROV h, DWORD param, DWORD flags )
{
	DWORD size = 0;
	if( !CryptGetProvParam( h, param, 0, &size, flags ) )
		return QByteArray();
	QByteArray data;
	data.resize( size );
	if( !CryptGetProvParam( h, param, (BYTE*)data.data(), &size, flags ) )
		return QByteArray();
	return data;
}



QCSP::QCSP( QObject *parent )
:	QObject( parent )
,	d( new QCSPPrivate )
{
}

QCSP::~QCSP()
{
	if( d->h )
		CryptReleaseContext( d->h, 0 );
	delete d;
}

QCSP::PinStatus QCSP::login( const TokenData &t )
{
	if( !d->h )
		return PinUnknown;

	PinDialog dialog( PinDialog::Pin2Type, t, qApp->activeWindow() );
	if( !dialog.exec() )
		return PinCanceled;
	return CryptSetProvParam( d->h, PP_SIGNATURE_PIN, (BYTE*)dialog.text().utf16(), 0 ) ? PinOK : PinUnknown;
}

QStringList QCSP::providers()
{
	QStringList result;
	HCRYPTPROV h = 0;
	DWORD index = 0, type = 0, size = 0;
	while( CryptEnumProvidersW( index, 0, 0, &type, 0, &size ) )
	{
		QScopedArrayPointer<wchar_t> provider( new wchar_t[size] );
		if( !CryptEnumProvidersW( index++, 0, 0, &type, provider.data(), &size ) ||
			type != PROV_RSA_FULL )
			continue;

		QString prov = QString::fromUtf16( (const ushort*)provider.data() );
		// its broken and does not play well with pkcs11
		if( prov.toLower().contains( "esteid" ) )
			continue;

		if( h )
			CryptReleaseContext( h, 0 );
		h = 0;
		if( !CryptAcquireContextW( &h, 0, provider.data(), type, CRYPT_SILENT ) )
			continue;

		QByteArray imptype = QCSPPrivate::provParam( h, PP_IMPTYPE );
		if( imptype.isEmpty() || !(imptype[0] & CRYPT_IMPL_HARDWARE) )
			continue;

		HCRYPTKEY key = 0;
		if( !CryptGetUserKey( h, AT_SIGNATURE, &key ) )
			continue;

		QSslCertificate cert( QCSPPrivate::keyParam( key, KP_CERTIFICATE, 0 ), QSsl::Der );
		CryptDestroyKey( key );

		if( !cert.isNull() )
			result << prov;
	}
	if( h )
		CryptReleaseContext( h, 0 );

	return result;
}

TokenData QCSP::selectProvider( const QString &provider, SslCertificate::KeyUsage usage )
{
	TokenData t;
	t.setCard( provider );

	if( d->h )
		CryptReleaseContext( d->h, 0 );
	if( !CryptAcquireContextW( &d->h, 0, (WCHAR*)provider.utf16(), PROV_RSA_FULL, 0 ) )
		return t;

	HCRYPTKEY key = 0;
	if( !CryptGetUserKey( d->h, AT_SIGNATURE, &key ) )
		return t;

	SslCertificate cert = QSslCertificate( d->keyParam( key, KP_CERTIFICATE, 0 ), QSsl::Der );
	CryptDestroyKey( key );
	if( cert.keyUsage().keys().contains( usage ) )
		t.setCert( cert );

	return t;
}

QByteArray QCSP::sign( int method, const QByteArray &digest )
{
	ALG_ID alg = 0;
	switch( method )
	{
	case NID_sha1: alg = CALG_SHA1; break;
	case NID_sha256: alg = CALG_SHA_256; break;
	case NID_sha384: alg = CALG_SHA_384; break;
	case NID_sha512: alg = CALG_SHA_512; break;
	case NID_sha224:
	default: return QByteArray();
	}

	HCRYPTHASH hash = 0;
	if( !CryptCreateHash( d->h, alg, 0, 0, &hash ) )
		return QByteArray();

	if( !CryptSetHashParam( hash, HP_HASHVAL, (BYTE*)digest.constData(), 0 ) )
	{
		CryptDestroyHash( hash );
		return QByteArray();
	}

	DWORD size = 256;
	QByteArray sig;
	sig.resize( size );
	if( !CryptSignHashW( hash, AT_SIGNATURE, 0, 0, (BYTE*)sig.data(), &size ) )
		sig.clear();
	CryptDestroyHash( hash );

	QByteArray reverse;
	for( QByteArray::const_iterator i = sig.constEnd(); i != sig.constBegin(); )
	{
		--i;
		reverse += *i;
	}

	return reverse;
}
