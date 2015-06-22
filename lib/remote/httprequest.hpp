/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "base/stream.hpp"
#include "remote/i2-remote.hpp"

namespace icinga
{

enum HttpVersion
{
	HttpVersion10,
	HttpVersion11
};

enum HttpRequestState
{
	HttpRequestStart,
	HttpRequestHeaders,
	HttpRequestBody
};

/**
 * An HTTP request.
 *
 * @ingroup remote
 */
struct I2_REMOTE_API HttpRequest
{
public:
	bool Complete;

	String RequestMethod;
	String Url;
	HttpVersion ProtocolVersion;

	int ContentLength;
	String TransferEncoding;
	bool CloseConnection;

	HttpRequest(void);

	bool Parse(const Stream::Ptr& stream, StreamReadContext& src, bool may_wait);

private:
	HttpRequestState m_State;
};

}

#endif /* HTTPREQUEST_H */
