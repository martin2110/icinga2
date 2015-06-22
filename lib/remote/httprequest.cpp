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

#include "remote/httprequest.hpp"
#include "base/logger.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

HttpRequest::HttpRequest(void)
    : m_State(HttpRequestStart), ContentLength(-1), ProtocolVersion(HttpVersion10),
      Complete(false), CloseConnection(true)
{ }

bool HttpRequest::Parse(const Stream::Ptr& stream, StreamReadContext& src, bool may_wait)
{
	String line;

	StreamReadStatus srs = stream->ReadLine(&line, src, may_wait);

	if (srs != StatusNewItem)
		return false;

	if (m_State == HttpRequestStart) {
		/* ignore trailing new-lines */
		if (line == "")
			return true;

		std::vector<String> tokens;
		boost::algorithm::split(tokens, line, boost::is_any_of(" "));
		Log(LogWarning, "HttpRequest")
		    << "line: " << line << ", tokens: " << tokens.size();
		if (tokens.size() != 3)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));
		RequestMethod = tokens[0];
		Url = tokens[1];

		if (tokens[2] == "HTTP/1.0")
			ProtocolVersion = HttpVersion10;
		else if (tokens[2] == "HTTP/1.1") {
			ProtocolVersion = HttpVersion11;
			CloseConnection = false;
		} else
			BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported HTTP version"));

		m_State = HttpRequestHeaders;
		Log(LogWarning, "HttpRequest")
		    << "Method: " << RequestMethod << ", Url: " << Url;
	} else if (m_State == HttpRequestHeaders) {
		Log(LogInformation, "HttpRequest", line);
		if (line == "") {
			m_State = HttpRequestBody;

			/* we're done if the request doesn't contain a message body */
			if (ContentLength == -1 && TransferEncoding == "") {
				Complete = true;
				return true;
			}

			Log(LogWarning, "HttpRequest", "Waiting for message body");
		}
	}

	return true;
}

