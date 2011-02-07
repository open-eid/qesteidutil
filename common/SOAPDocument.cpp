/*
 * QEstEidCommon
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

#include "SOAPDocument.h"

#include <QBuffer>
#include <QVariant>

SOAPDocument::SOAPDocument( const QString &action, const QString &namespaceUri )
:	QXmlStreamWriter()
,	data( new QBuffer() )
{
	data->open( QBuffer::WriteOnly );
	setDevice( data );
	setAutoFormatting( true );

	writeStartDocument();
	writeNamespace( XML_SCHEMA, "xsd" );
	writeNamespace( XML_SCHEMA_INSTANCE, "xsi" );
	writeNamespace( SOAP_ENV, "SOAP-ENV" );
	writeNamespace( SOAP_ENC, "SOAP-ENC" );
	writeStartElement( SOAP_ENV, "Envelope" );

	writeStartElement( SOAP_ENV, "Body" );

	writeNamespace( namespaceUri, "m" );
	writeStartElement( namespaceUri, action );
	writeAttribute( SOAP_ENV, "encodingStyle", SOAP_ENC );
}

SOAPDocument::~SOAPDocument() { delete data; }

QByteArray SOAPDocument::document() const { return data->data(); }

void SOAPDocument::finalize()
{
	writeEndElement(); // action

	writeEndElement(); // Body

	writeEndElement(); // Envelope
	writeEndDocument();
}

void SOAPDocument::writeParameter( const QString &name, const QVariant &value )
{
	QString type;
	switch( value.type() )
	{
	case QMetaType::Bool: type = "boolean"; break;
	case QMetaType::Int: type = "int"; break;
	case QMetaType::QByteArray:
	case QMetaType::QString:
	default: type = "string"; break;
	}

	writeStartElement( name );
	writeAttribute( XML_SCHEMA_INSTANCE, "type", QString( "xsd:" ).append( type ) );
	writeCharacters( value.toString() );
	writeEndElement();
}
