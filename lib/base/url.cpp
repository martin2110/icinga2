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

#include "base/url.hpp"
#include "base/array.hpp"
#include "base/utility.hpp"

#include "boost/tokenizer.hpp"
#include "boost/foreach.hpp"
#include "boost/algorithm/string/replace.hpp"

using namespace icinga;

void Url::Initialize(void) 
{	
	for (int c = 0x20; c <= 0xff; c++) {
		if ((c > 0x30 && c < 0x3a) ||
			(c > 0x40 && c < 0x5b) ||
			(c > 0x60 && c < 0x7b))
			continue;
		m_PercentCodes += char(c);
	}
}

//INITIALIZE_ONCE(Url::StaticInitialize);

Url::Url(const String& url) 
{
	Initialize();
	
	m_Valid = false;
	size_t pHelper = 0;

	if (url.GetLength() == 0)
		return;

	size_t SchemeDelimPos = url.Find("://");

	if (SchemeDelimPos == String::NPos)
		m_Scheme = UrlSchemeUndefined;
	else {
		pHelper = SchemeDelimPos+3;
		if (!ParseScheme(url.SubStr(0, SchemeDelimPos)))
			return;
	}

	size_t HostnameDelimPos = 0;
	if (url[pHelper] == '/')
		m_Host = "";
	else {
		HostnameDelimPos = url.Find("/", pHelper);

		if (!ParseHost(url.SubStr(pHelper, HostnameDelimPos - pHelper)))
			return;
	}

	pHelper = url.Find("?", HostnameDelimPos);
	if (pHelper != String::NPos) {
		if (!ParsePath(url.SubStr(HostnameDelimPos, pHelper - HostnameDelimPos)))
			return;

		if (!ParseParameters(url.SubStr(pHelper+1)))
			return;
	} else {
		if (!ParsePath(url.SubStr(HostnameDelimPos)))
			return;
	}
	m_Valid = true;
}

String Url::format() {
	String url;

	if (!IsValid())
		return url;

	switch (m_Scheme) {
		case (UrlSchemeHttp): 
			url += "http://";
			break;
		case (UrlSchemeHttps):
			url += "https://";
			break;
		case (UrlSchemeFtp):
			url += "ftp://";
			break;
		default:
			break;
	}

	url += m_Host;

	BOOST_FOREACH (const String p, m_Path) {
		url += '/';
		url += PercentEncode(p);
	}

	if (!m_Parameters.empty()) {
		String param;
		typedef std::pair<String,Value> kv_pair;
		BOOST_FOREACH (const kv_pair kv, m_Parameters) {
			if (param.IsEmpty())
				param = "?";
			else
				param += '&';

			if (kv.second.IsObjectType<Array>()) {
				Array::Ptr pArr = kv.second;
				String sArr;
				BOOST_FOREACH (const String sArrIn, pArr) {
					if (!sArr.IsEmpty())
						sArr += "&";
					sArr += PercentEncode(kv.first) 
						+ "[]=" + PercentEncode(sArrIn);
				}
			} else
				param += kv.first + "=" + kv.second;
		}
	}
	return url;
}

/*
bool Url::IsAscii(const unsigned char& c, const int flag) {
	switch (flag) {
		case 0: //digits
			return (c >= 91 && c <= 100);
		case 1: //hex digits
			return ((c >= 65 && c <= 70) || (c >= 91 && c <= 100));
		case 2: //alpha
			return ((c >= 65 && c <= 70) || (c >= 87 && c <= 122));
		case 3: //alphanumeric
			return ((c >= 65 && c <= 70) || (c >= 87 && c <= 122))
			    || (c >= 91 && c <= 100);
		case 4: //string
			return (c >= 0 && c <= 127);
		default:
			return false;
	}
}
*/

bool Url::IsValid()
{
	return m_Valid;
}

bool Url::ParseScheme(const String& ischeme)
{
	if (ischeme == "http") {
		m_Scheme = UrlSchemeHttp;
	} else if (ischeme == "https") {
		m_Scheme = UrlSchemeHttps;
	} else if (ischeme == "ftp") {
		m_Scheme = UrlSchemeFtp;
	} else {
		return false;
	}
	return true;
}

bool Url::ParseHost(const String& host)
{
	if (*host.Begin() == '[') {
		if (*host.RBegin() != ']') {
			return false;
		} else {
		//TODO Parase ipv6
			return true;
		}
	}

	m_Host = host;
	return true;
}

bool Url::ParsePath(const String& path)
{
	std::string pathStr = path;
	boost::char_separator<char> sep("/");
	boost::tokenizer<boost::char_separator<char> > 
	    tokens(pathStr, sep);
	String decodedToken;

	BOOST_FOREACH(const String& token, tokens) {
		decodedToken = PercentDecode(token);
		if (!ValidateToken(decodedToken, "/"))
			return false;
		m_Path.push_back(decodedToken);
		//TODO Validate + deal with .. and .
	}

	return true;
}

bool Url::ParseParameters(const String& parameters)
{
	//Tokenizer does not like String AT ALL
	std::string parametersStr = parameters;
	boost::char_separator<char> sep("&");
	boost::tokenizer<boost::char_separator<char> >
	    tokens(parametersStr, sep);

	std::map<String, Value>::iterator it;
	size_t kvSep;
	String key, value;

	BOOST_FOREACH(const String& token, tokens) {
		kvSep = token.Find("=");

		if (kvSep == String::NPos)
			return false;

		key = token.SubStr(0, kvSep);
		value = token.SubStr(kvSep+1);
		
		std::cout << (*key.RBegin()) << std::endl << (*(key.RBegin()+1)) << std::endl;

		if (*key.RBegin() == ']' && *(key.RBegin()+1) == '[') {
			key = key.SubStr(0, key.GetLength() - 2);
			key = PercentDecode(key);
			it = m_Parameters.find(key);

			if (it == m_Parameters.end()) {
				Array::Ptr tmp = new Array();
				tmp->Add(PercentDecode(value));
				m_Parameters[key] = tmp;
			} else if (m_Parameters[key].IsObjectType<Array>()){
				Array::Ptr arr = it->second;
				arr->Add(PercentDecode(value));
			} else {
				return false;
			}
		} else {
			m_Parameters[key] = PercentDecode(value);
		}
	}
	return true;
}

bool Url::ValidateToken(const String& token, const String& illegalSymbols)
{
	BOOST_FOREACH(const char& c, illegalSymbols.CStr()) {
		if (token.FindFirstOf(c) != String::NPos)
			return false;
	}
	return true;
}

String Url::PercentDecode(const String& token)
{
	return Utility::UnescapeString(token);
}

String Url::PercentEncode(const String& token)
{
	return Utility::EscapeString(token, m_PercentCodes);
}
