/*
 * QDigiDocCommon
 *
 * Copyright (C) 2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010 Raul Metsma <raul@innovaatik.ee>
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

#include "QPKCS11_p.h"

#include "PinDialog.h"

#include <QApplication>
#include <QStringList>

bool QPKCS11Private::attribute( CK_OBJECT_HANDLE obj, CK_ATTRIBUTE_TYPE type, void *value, unsigned long &size )
{
	CK_ATTRIBUTE attr = { type, value, size };
	err = f->C_GetAttributeValue( session, obj, &attr, 1 );
	size = attr.ulValueLen;
	return err == CKR_OK;
}

QSslCertificate QPKCS11Private::readCert( CK_SLOT_ID slot )
{
	if( session > 0 )
		f->C_CloseSession( session );
	err = f->C_OpenSession( slot, CKF_SERIAL_SESSION, 0, 0, &session );
	if( err != CKR_OK )
		return QSslCertificate();

	CK_OBJECT_HANDLE obj = CK_INVALID_HANDLE;
	if( !findObject( CKO_CERTIFICATE, &obj ) || obj == CK_INVALID_HANDLE )
		return QSslCertificate();

	unsigned long size = 0;
	if( !attribute( obj, CKA_VALUE, 0, size ) )
		return QSslCertificate();

	char *cert_data = new char[size];
	if( !attribute( obj, CKA_VALUE, (unsigned char*)cert_data, size ) )
	{
		delete [] cert_data;
		return QSslCertificate();
	}

	QSslCertificate cert = QSslCertificate( QByteArray( cert_data, size ), QSsl::Der );
	delete [] cert_data;
	return cert;
}

bool QPKCS11Private::findObject( CK_OBJECT_CLASS cls, CK_OBJECT_HANDLE *ret )
{
	*ret = CK_INVALID_HANDLE;

	CK_ATTRIBUTE attr = { CKA_CLASS, &cls, sizeof(cls) };
	if( (err = f->C_FindObjectsInit( session, &attr, 1 )) != CKR_OK )
		return false;

	CK_ULONG count = 0;
	err = f->C_FindObjects( session, ret, 1, &count );
	f->C_FindObjectsFinal( session );
	return err == CKR_OK;
}

void QPKCS11Private::freeSlotIds()
{
	delete [] pslots;
	pslots = 0;
	nslots = 0;
}

bool QPKCS11Private::getSlotsIds()
{
	freeSlotIds();
	if( (err = f->C_GetSlotList( true, 0, &nslots )) != CKR_OK )
		return false;
	pslots = new CK_SLOT_ID[nslots];
	if( (err = f->C_GetSlotList( true, pslots, &nslots )) == CKR_OK )
		return true;
	freeSlotIds();
	return false;
}



QPKCS11::QPKCS11( QObject *parent )
:	QObject( parent )
,	d( new QPKCS11Private )
{
}

QPKCS11::~QPKCS11() { unloadDriver(); delete d; }

QStringList QPKCS11::cards()
{
	if( !d->getSlotsIds() )
		return QStringList();

	QStringList cards;
	for( unsigned int i = 0; i < d->nslots; ++i )
	{
		CK_TOKEN_INFO token;
		d->err = d->f->C_GetTokenInfo( d->pslots[i], &token );
		cards << QByteArray( (const char*)token.serialNumber, 16 ).trimmed();
	}
	cards.removeDuplicates();
	return cards;
}

bool QPKCS11::encrypt( const QByteArray &data, unsigned char *encrypted, unsigned long *len )
{
	CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
	if( !d->findObject( CKO_PRIVATE_KEY, &key ) || key == CK_INVALID_HANDLE )
		return false;

	CK_MECHANISM mech = { CKM_RSA_PKCS, 0, 0 };
	if( (d->err = d->f->C_EncryptInit( d->session, &mech, key )) != CKR_OK )
		return false;

	if( (d->err = d->f->C_Encrypt( d->session, (unsigned char*)data.constData(), data.size(), 0, len )) != CKR_OK )
		return false;

	d->err = d->f->C_Encrypt( d->session, (unsigned char*)data.constData(), data.size(), encrypted, len );
	return d->err == CKR_OK;
}

bool QPKCS11::decrypt( const QByteArray &data, unsigned char *decrypted, unsigned long *len )
{
	CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
	if( !d->findObject( CKO_PRIVATE_KEY, &key ) || key == CK_INVALID_HANDLE )
		return false;

	CK_MECHANISM mech = { CKM_RSA_PKCS, 0, 0 };
	if( (d->err = d->f->C_DecryptInit( d->session, &mech, key )) != CKR_OK )
		return false;

	if( (d->err = d->f->C_Decrypt( d->session, (unsigned char*)data.constData(), data.size(), 0, len )) != CKR_OK )
		return false;

	return (d->err = d->f->C_Decrypt( d->session, (unsigned char*)data.constData(), data.size(), decrypted, len )) == CKR_OK;
}

bool QPKCS11::loadDriver( const QString &driver )
{
	d->lib.setFileName( driver );
	CK_C_GetFunctionList l = CK_C_GetFunctionList(d->lib.resolve( "C_GetFunctionList" ));
	if( !l )
		return false;

	d->err = l( &d->f );
	if( d->err != CKR_OK )
		return false;

	return (d->err = d->f->C_Initialize( 0 )) == CKR_OK;
}

QPKCS11::PinStatus QPKCS11::login( const TokenData &_t )
{
	CK_TOKEN_INFO token;
	if( !d->pslot || (d->err = d->f->C_GetTokenInfo( *(d->pslot), &token )) != CKR_OK )
		return PinUnknown;

	if( !(token.flags & CKF_LOGIN_REQUIRED) )
		return PinOK;

	TokenData t = _t;
	if( token.flags & CKF_SO_PIN_COUNT_LOW || token.flags & CKF_USER_PIN_COUNT_LOW )
		t.setFlag( TokenData::PinCountLow );
	if( token.flags & CKF_SO_PIN_FINAL_TRY || token.flags & CKF_USER_PIN_FINAL_TRY )
		t.setFlag( TokenData::PinFinalTry );
	if( token.flags & CKF_SO_PIN_LOCKED || token.flags & CKF_USER_PIN_LOCKED )
		t.setFlag( TokenData::PinLocked );

	if( d->session )
		d->err = d->f->C_CloseSession( d->session );
	if( (d->err = d->f->C_OpenSession( *(d->pslot), CKF_SERIAL_SESSION, 0, 0, &d->session )) != CKR_OK )
		return PinUnknown;

	bool pin2 = SslCertificate( t.cert() ).keyUsage().keys().contains( SslCertificate::NonRepudiation );
	if( token.flags & CKF_PROTECTED_AUTHENTICATION_PATH )
	{
		PinDialog p( pin2 ? PinDialog::Pin2PinpadType : PinDialog::Pin1PinpadType, t, qApp->activeWindow() );
		p.open();
		d->err = QPKCS11Thread( d ).waitForDone();
	}
	else
	{
		PinDialog p( pin2 ? PinDialog::Pin2Type : PinDialog::Pin1Type, t, qApp->activeWindow() );
		if( !p.exec() )
			return PinCanceled;
		QByteArray pin = p.text().toUtf8();
		d->err = d->f->C_Login( d->session, CKU_USER, (unsigned char*)pin.constData(), pin.size() );
	}

	switch( d->err )
	{
	case CKR_OK: return PinOK;
	case CKR_CANCEL:
	case CKR_FUNCTION_CANCELED: return PinCanceled;
	case CKR_PIN_INCORRECT: return PinIncorrect;
	case CKR_PIN_LOCKED: return PinLocked;
	default: return PinUnknown;
	}
}

bool QPKCS11::logout() { return (d->err = d->f->C_Logout( d->session )) == CKR_OK; }

TokenData QPKCS11::selectSlot( const QString &card, SslCertificate::KeyUsage usage )
{
	delete d->pslot;
	d->pslot = 0;
	TokenData t;
	t.setCard( card );
	for( unsigned int i = 0; i < d->nslots; ++i )
	{
		CK_TOKEN_INFO token;
		if( (d->err = d->f->C_GetTokenInfo( d->pslots[i], &token )) != CKR_OK ||
			card != QByteArray( (const char*)token.serialNumber, 16 ).trimmed() )
			continue;

		SslCertificate cert = d->readCert( d->pslots[i] );
		if( !cert.keyUsage().keys().contains( usage ) )
			continue;

		d->pslot = new CK_SLOT_ID( d->pslots[i] );
		t.setCert( cert );
		if( token.flags & CKF_SO_PIN_COUNT_LOW || token.flags & CKF_USER_PIN_COUNT_LOW )
			t.setFlag( TokenData::PinCountLow );
		if( token.flags & CKF_SO_PIN_FINAL_TRY || token.flags & CKF_USER_PIN_FINAL_TRY )
			t.setFlag( TokenData::PinFinalTry );
		if( token.flags & CKF_SO_PIN_LOCKED || token.flags & CKF_USER_PIN_LOCKED )
			t.setFlag( TokenData::PinLocked );
		break;
	}
	return t;
}

bool QPKCS11::sign( const QByteArray &data, unsigned char *signature, unsigned long *len )
{
	CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
	if( !d->findObject( CKO_PRIVATE_KEY, &key ) || key == CK_INVALID_HANDLE )
		return false;

	CK_MECHANISM mech = { CKM_RSA_PKCS, 0, 0 };
	if( (d->err = d->f->C_SignInit( d->session, &mech, key )) != CKR_OK )
		return false;

	if( (d->err = d->f->C_Sign( d->session, (unsigned char*)data.constData(), data.size(), 0, len )) != CKR_OK )
		return false;

	return (d->err = d->f->C_Sign( d->session, (unsigned char*)data.constData(), data.size(), signature, len )) == CKR_OK;
}

void QPKCS11::unloadDriver()
{
	d->freeSlotIds();
	delete d->pslot;
	d->pslot = 0;
	if( d->f )
		d->f->C_Finalize( 0 );
	d->f = 0;
	d->lib.unload();
}

bool QPKCS11::verify( const QByteArray &data, unsigned char *signature, unsigned long len )
{
	CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
	if( !d->findObject( CKO_PRIVATE_KEY, &key ) || key == CK_INVALID_HANDLE )
		return false;

	CK_MECHANISM mech = { CKM_RSA_PKCS, 0, 0 };
	if( (d->err = d->f->C_VerifyInit( d->session, &mech, key )) != CKR_OK )
		return false;

	if( (d->err = d->f->C_Verify( d->session, (unsigned char*)data.constData(), data.size(), 0, len )) != CKR_OK )
		return false;

	return (d->err = d->f->C_Verify( d->session, (unsigned char*)data.constData(), data.size(), signature, len )) == CKR_OK;
}
