/*
 * QEstEidCommon
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

#include "QPCSC.h"
#include "QPCSC_p.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include <cstring>

#ifdef Q_OS_WIN
#include <Regstr.h>
#include <Setupapi.h>
#endif

QPCSCReaderPrivate::QPCSCReaderPrivate( QPCSCPrivate *_d )
	: d( _d )
	, card( 0 )
	, proto( 0 )
{
	std::memset( &state, 0, sizeof(state) );
}

QByteArray QPCSCReaderPrivate::attrib( DWORD id ) const
{
	DWORD size = 0;
	LONG err = SCardGetAttrib( card, id, 0, &size );
	if( err != SCARD_S_SUCCESS || !size )
		return QByteArray();
	QByteArray data( size, 0 );
	err = SCardGetAttrib( card, id, LPBYTE(data.data()), &size );
	if( err != SCARD_S_SUCCESS || !size )
		return QByteArray();
	return data;
}



QPCSC::QPCSC( QObject *parent )
	: QObject( parent )
	, d( new QPCSCPrivate )
{
	DWORD err = SCardEstablishContext( SCARD_SCOPE_USER, 0, 0, &d->context );
	d->running = err != SCARD_E_NO_SERVICE;
}

QPCSC::~QPCSC()
{
	qDeleteAll( findChildren<QPCSCReader*>() );
	if( d->context )
		SCardReleaseContext( d->context );
	delete d;
}

QStringList QPCSC::drivers() const
{
#ifdef Q_OS_WIN
	HDEVINFO h = SetupDiGetClassDevs( 0, 0, 0, DIGCF_ALLCLASSES | DIGCF_PRESENT );
	if( !h )
		return QStringList();

	SP_DEVINFO_DATA info = { sizeof(SP_DEVINFO_DATA) };
	QStringList list;
	for( DWORD i = 0; SetupDiEnumDeviceInfo( h, i, &info ); i++ )
	{
		DWORD size = 0;
		WCHAR data[1024];

		SetupDiGetDeviceRegistryPropertyW( h, &info,
			SPDRP_CLASS, 0, LPBYTE(data), sizeof(data), &size );
		if( _wcsicmp( data, L"SmartCardReader" ) != 0 )
			continue;

		DWORD conf = 0;
		SetupDiGetDeviceRegistryPropertyW( h, &info,
			SPDRP_CONFIGFLAGS, 0, LPBYTE(&conf), sizeof(conf), &size );
		if( conf & CONFIGFLAG_DISABLED )
			continue;

		SetupDiGetDeviceRegistryPropertyW( h, &info,
			SPDRP_DEVICEDESC, 0, LPBYTE(data), sizeof(data), &size );
		QString name( (QChar*)data );

		SetupDiGetDeviceRegistryPropertyW( h, &info,
			SPDRP_HARDWAREID, 0, LPBYTE(data), sizeof(data), &size );

		list << QString( "%1 (%2)").arg( name, QString( (QChar*)data ) );
	}
	SetupDiDestroyDeviceInfoList( h );

	return list;
#else
	return readers();
#endif
}

QStringList QPCSC::readers() const
{
	if( !d->context )
		return QStringList();

	DWORD size = 0;
	LONG err = SCardListReaders( d->context, 0, 0, &size );
	if( err != SCARD_S_SUCCESS || !size )
		return QStringList();

	QByteArray data( size, 0 );
	err = SCardListReaders( d->context, 0, data.data(), &size );
	if( err != SCARD_S_SUCCESS )
		return QStringList();

	data.resize( size );
	QStringList readers;
	for( QByteArray::const_iterator i = data.constBegin(); i != data.constEnd(); ++i )
	{
		QString reader( i );
		if( !reader.isEmpty() )
			readers << reader;
		i += reader.size() + 1;
	}

	return readers;
}

bool QPCSC::serviceRunning() const
{
	return d->running;
}



QPCSCReader::QPCSCReader( const QString &reader, QPCSC *parent )
	: QObject( parent )
	, d( new QPCSCReaderPrivate( parent->d ) )
{
	d->reader = reader.toUtf8();
	d->state.szReader = d->reader.constData();
	updateState();
}

QPCSCReader::~QPCSCReader()
{
	disconnect();
	delete d;
}

QByteArray QPCSCReader::atr() const
{
	return QByteArray( (const char*)d->state.rgbAtr, d->state.cbAtr ).toHex().toUpper();
}

bool QPCSCReader::beginTransaction()
{
	return SCardBeginTransaction( d->card ) == SCARD_S_SUCCESS;
}

bool QPCSCReader::connect( Connect connect, Mode mode )
{
	if( !d->d->context )
		return false;
	LONG err = SCardConnect( d->d->context, d->state.szReader, connect, mode, &d->card, &d->proto );
	updateState();
	d->friendlyName = d->attrib( SCARD_ATTR_DEVICE_FRIENDLY_NAME_A );
#if 0
	qDebug() << "SCARD_ATTR_DEVICE_FRIENDLY_NAME:" << d->attrib( SCARD_ATTR_DEVICE_FRIENDLY_NAME_A );
	qDebug() << "SCARD_ATTR_DEVICE_SYSTEM_NAME:" << d->attrib( SCARD_ATTR_DEVICE_SYSTEM_NAME_A );
	qDebug() << "SCARD_ATTR_DEVICE_UNIT:" << d->attrib( SCARD_ATTR_DEVICE_UNIT );
	qDebug() << "SCARD_ATTR_VENDOR_IFD_SERIAL_NO:" << d->attrib( SCARD_ATTR_VENDOR_IFD_SERIAL_NO );
	qDebug() << "SCARD_ATTR_VENDOR_IFD_TYPE:" << d->attrib( SCARD_ATTR_VENDOR_IFD_TYPE );
	qDebug() << "SCARD_ATTR_VENDOR_IFD_VERSION:" << d->attrib( SCARD_ATTR_VENDOR_IFD_VERSION );
	qDebug() << "SCARD_ATTR_VENDOR_NAME:" << d->attrib( SCARD_ATTR_VENDOR_NAME );
#endif

	DWORD size = 0;
	BYTE feature[256];
	LONG rv = SCardControl( d->card, CM_IOCTL_GET_FEATURE_REQUEST, 0, 0, feature, sizeof(feature), &size );
	if( rv == SCARD_S_SUCCESS && (size % sizeof(PCSC_TLV_STRUCTURE)) == 0 )
	{
		size /= sizeof(PCSC_TLV_STRUCTURE);
		PCSC_TLV_STRUCTURE *pcsc_tlv = (PCSC_TLV_STRUCTURE *)feature;
		for( DWORD i = 0; i < size; i++ )
			d->ioctl[DRIVER_FEATURES(pcsc_tlv[i].tag)] = ntohl( pcsc_tlv[i].value );
	}

	return err == SCARD_S_SUCCESS;
}

void QPCSCReader::disconnect( Reset reset )
{
	if( d->card )
		SCardDisconnect( d->card, reset );
	d->proto = 0;
	d->card = 0;
	d->ioctl.clear();
	updateState();
}

bool QPCSCReader::endTransaction( Reset reset )
{
	return SCardEndTransaction( d->card, reset ) == SCARD_S_SUCCESS;
}

QString QPCSCReader::friendlyName() const
{
	return QString::fromLocal8Bit( d->friendlyName );
}

bool QPCSCReader::isPinPad()
{
	if( d->card )
		return d->ioctl.contains(FEATURE_VERIFY_PIN_DIRECT) || d->ioctl.contains(FEATURE_VERIFY_PIN_START);

	if( !isPresent() )
	{
#ifndef Q_OS_WIN /* Apple 10.5.7 and pcsc-lite previous to v1.5.5 do not support 0 as protocol identifier */
		Mode mode = Mode(T0|T1);
#else
		Mode mode = Undefined;
#endif
		if( connect( Direct, mode ) )
			return false;
	}
	else if( !connect() )
		return false;
	bool result = d->ioctl.contains(FEATURE_VERIFY_PIN_DIRECT) || d->ioctl.contains(FEATURE_VERIFY_PIN_START);
	disconnect();
	return result;
}

bool QPCSCReader::isPresent() const
{
	return d->state.dwEventState & SCARD_STATE_PRESENT;
}

QString QPCSCReader::name() const
{
	return QString::fromLocal8Bit( d->reader );
}

QHash<QPCSCReader::Properties, int> QPCSCReader::properties() const
{
	QHash<Properties,int> properties;
	if( DWORD ioctl = d->ioctl.value(FEATURE_GET_TLV_PROPERTIES) )
	{
		DWORD size = 0;
		BYTE recv[256];
		DWORD rv = SCardControl( d->card, ioctl, 0, 0, recv, sizeof(recv), &size );
		Q_UNUSED(rv);
		unsigned char *p = recv;
		while( DWORD(p-recv) < size )
		{
			int tag = *p++, len = *p++, value = -1;
			switch( len )
			{
			case 1: value = *p; break;
			case 2: value = *p + (*(p+1)<<8); break;
			case 4: value = *p + (*(p+1)<<8) + (*(p+2)<<16) + (*(p+3)<<24); break;
			default: break;
			}
			p += len;
			properties[Properties(tag)] = value;
		}
	}
	return properties;
}

int QPCSCReader::protocol() const
{
	return d->proto;
}

bool QPCSCReader::reconnect( Reset reset, Mode mode )
{
	if( !d->card )
		return false;
	LONG err = SCardReconnect( d->card, SCARD_SHARE_SHARED, mode, reset, &d->proto );
	updateState();
	return err == SCARD_S_SUCCESS;
}

QStringList QPCSCReader::state() const
{
	QStringList result;
#define STATE(X) if( d->state.dwEventState & SCARD_STATE_##X ) result << #X
	STATE(IGNORE);
	STATE(CHANGED);
	STATE(UNKNOWN);
	STATE(UNAVAILABLE);
	STATE(EMPTY);
	STATE(PRESENT);
	STATE(ATRMATCH);
	STATE(EXCLUSIVE);
	STATE(INUSE);
	STATE(MUTE);
	return result;
}

QPCSCReader::Result QPCSCReader::transfer( const char *cmd, int size ) const
{
	return transfer( QByteArray( cmd, size ) );
}

QPCSCReader::Result QPCSCReader::transfer( const QByteArray &cmd ) const
{
	static const SCARD_IO_REQUEST T0 = { 1, 8 };
	static const SCARD_IO_REQUEST T1 = { 2, 8 };
	QByteArray data( 255 + 3, 0 );
	DWORD size = data.size();

	qDebug().nospace() << ">(T=" << qint8(d->proto == SCARD_PROTOCOL_RAW ? -1 : d->proto - 1) << "): " << cmd.toHex();
	DWORD ret = SCardTransmit( d->card, d->proto == SCARD_PROTOCOL_T0 ? &T0 : &T1,
		LPCBYTE(cmd.constData()), cmd.size(), 0, LPBYTE(data.data()), &size );
	if( ret != SCARD_S_SUCCESS )
	{
		qDebug() << "Err:" << ret;
		return Result();
	}

	Result result = { data.mid( size-2, 2 ), data.left( size - 2 ) };
	qDebug().nospace() << "<(T=" << qint8(d->proto == SCARD_PROTOCOL_RAW ? -1 : d->proto - 1) << "): " << result.status.toHex() << result.data.toHex();

	if( result.status.at( 0 ) == 0x61 )
	{
		QByteArray cmd( "\x00\xC0\x00\x00\x00", 5 );
		cmd[4] = data.at( size-1 );
		return transfer( cmd );
	}
	return result;
}

bool QPCSCReader::updateState( quint32 msec )
{
	if(!d->d->context)
		return false;
	d->state.dwCurrentState = d->state.dwEventState; //(currentReaderCount << 16)
	DWORD err = SCardGetStatusChange( d->d->context, msec, &d->state, 1); //INFINITE
	switch(err) {
	case SCARD_S_SUCCESS: return true;
	case SCARD_E_TIMEOUT: return msec == 0;
	default: return false;
	}
}
