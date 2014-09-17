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

#pragma once

#include <QtCore/QHash>

#ifdef Q_OS_WIN
#undef UNICODE
#define NOMINMAX
#include <winsock2.h>
#include <winscard.h>
#else
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#include <arpa/inet.h>
#endif

#ifndef SCARD_CTL_CODE
#define SCARD_CTL_CODE(code) (0x42000000 + (code))
#endif

// http://www.pcscworkgroup.com/specifications/files/pcsc10_v2.02.09.pdf
#define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)

enum DRIVER_FEATURES {
FEATURE_VERIFY_PIN_START         = 0x01,
FEATURE_VERIFY_PIN_FINISH        = 0x02,
FEATURE_MODIFY_PIN_START         = 0x03,
FEATURE_MODIFY_PIN_FINISH        = 0x04,
FEATURE_GET_KEY_PRESSED          = 0x05,
FEATURE_VERIFY_PIN_DIRECT        = 0x06,
FEATURE_MODIFY_PIN_DIRECT        = 0x07,
FEATURE_MCT_READER_DIRECT        = 0x08,
FEATURE_MCT_UNIVERSAL            = 0x09,
FEATURE_IFD_PIN_PROPERTIES       = 0x0A,
FEATURE_ABORT                    = 0x0B,
FEATURE_SET_SPE_MESSAGE          = 0x0C,
FEATURE_VERIFY_PIN_DIRECT_APP_ID = 0x0D,
FEATURE_MODIFY_PIN_DIRECT_APP_ID = 0x0E,
FEATURE_WRITE_DISPLAY            = 0x0F,
FEATURE_GET_KEY                  = 0x10,
FEATURE_IFD_DISPLAY_PROPERTIES   = 0x11,
FEATURE_GET_TLV_PROPERTIES       = 0x12,
FEATURE_CCID_ESC_COMMAND         = 0x13
};

#ifdef Q_OS_MAC
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

typedef struct
{
	quint8 tag;
	quint8 length;
	quint32 value;
} PCSC_TLV_STRUCTURE;

typedef struct
{
	quint8 bTimerOut;
	quint8 bTimerOut2;
	quint8 bmFormatString;
	quint8 bmPINBlockString;
	quint8 bmPINLengthFormat;
	quint16 wPINMaxExtraDigit;
	quint8 bEntryValidationCondition;
	quint8 bNumberMessage;
	quint16 wLangId;
	quint8 bMsgIndex;
	quint8 bTeoPrologue[3];
	quint32 ulDataLength;
	quint8 abData[1];
} PIN_VERIFY_STRUCTURE;

typedef struct
{
	quint8 bTimerOut;
	quint8 bTimerOut2;
	quint8 bmFormatString;
	quint8 bmPINBlockString;
	quint8 bmPINLengthFormat;
	quint8 bInsertionOffsetOld;
	quint8 bInsertionOffsetNew;
	quint16 wPINMaxExtraDigit;
	quint8 bConfirmPIN;
	quint8 bEntryValidationCondition;
	quint8 bNumberMessage;
	quint16 wLangId;
	quint8 bMsgIndex1;
	quint8 bMsgIndex2;
	quint8 bMsgIndex3;
	quint8 bTeoPrologue[3];
	quint32 ulDataLength;
	quint8 abData[1];
} PIN_MODIFY_STRUCTURE;

typedef struct {
	quint16 wLcdLayout;
	quint8 bEntryValidationCondition;
	quint8 bTimeOut2;
} PIN_PROPERTIES_STRUCTURE;

typedef struct {
	quint16 wLcdMaxCharacters;
	quint16 wLcdMaxLines;
} DISPLAY_PROPERTIES_STRUCTURE;

#ifdef Q_OS_MAC
#pragma pack()
#else
#pragma pack(pop)
#endif

#ifndef SCARD_ATTR_DEVICE_FRIENDLY_NAME_A
#define SCARD_ATTR_VALUE(Class, Tag) ((((ULONG)(Class)) << 16) | ((ULONG)(Tag)))
#define SCARD_CLASS_SYSTEM     0x7fff
#define SCARD_ATTR_DEVICE_FRIENDLY_NAME_A SCARD_ATTR_VALUE(SCARD_CLASS_SYSTEM, 0x0003)
#define SCARD_ATTR_DEVICE_FRIENDLY_NAME SCARD_ATTR_DEVICE_FRIENDLY_NAME_A
#endif

class QPCSCPrivate
{
public:
	QPCSCPrivate(): context(0), running(false) {}

	SCARDCONTEXT context;
	bool running;
};

class QPCSCReaderPrivate
{
public:
	QPCSCReaderPrivate( QPCSCPrivate *d );

	QByteArray attrib( DWORD id ) const;

	QPCSCPrivate *d;
	SCARDHANDLE card;
	DWORD proto;
	SCARD_READERSTATE state;
	QByteArray reader, friendlyName;

	QHash<DRIVER_FEATURES,DWORD> ioctl;
};
