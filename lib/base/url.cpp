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

#define ALPHA "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMERIC "0123456789"

void Url::Initialize(void) 
{	
	//unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
	m_UnreservedCharacters = 
		String(ALPHA) + String(NUMERIC) + "-._~";
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
		m_Scheme = "";
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

	pHelper = url.FindFirstOf("#?", HostnameDelimPos);

	if (!ParsePath(url.SubStr(HostnameDelimPos, pHelper - HostnameDelimPos)))
		return;

	if (url[pHelper] == '?') {
		if (!ParseParameters(url.SubStr(pHelper+1)))
			return;
	}


	m_Valid = true;
}

String Url::GetHost(void) {
	return m_Host;
}

String Url::GetScheme(void){
	return m_Scheme;
}

std::map<String,Value> Url::GetParameters(void) {
	return m_Parameters;
}

Value Url::GetParameter(const String& name) {
	std::map<String,Value>::iterator it = m_Parameters.find(name);

	if (it == m_Parameters.end())
		return Empty;

	return it->second;
}

std::vector<String> Url::GetPath(void) {
	return m_Path;
}

bool Url::IsValid()
{
	return m_Valid;
}

String Url::Format() {

	if (!IsValid())
		return "";

	String url = "";

	if (!m_Scheme.IsEmpty())
		url += m_Scheme + "://";

	url += m_Host;

	BOOST_FOREACH (const String p, m_Path) {
		url += '/';
		url += PercentEncode(p);
	}

	String param = "";
	if (!m_Parameters.empty()) {
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
	
	url += param;

	return url;
}

bool Url::ParseScheme(const String& ischeme)
{
	m_Scheme = ischeme;

	if (ischeme.FindFirstOf(ALPHA) != 0)
		return false;

	return (ValidateToken(ischeme, String(ALPHA) + String(NUMERIC) + "+-.", false));
}

bool Url::ParseHost(const String& host)
{	
	m_Host = host;

	return (ValidateToken(host, String(ALPHA) + String(NUMERIC) + ".-", false));
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

		if (!ValidateToken(decodedToken, "/", true))
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
		
		if (key.IsEmpty() || value.IsEmpty())
			return false;

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
			} else
				return false;
		} else {
			key = PercentDecode(key);
			if (m_Parameters.find(key) == m_Parameters.end())
				m_Parameters[key] = PercentDecode(value);
			else
				return false;
		}
	}

	return true;
}

bool Url::ValidateToken(const String& token, const String& symbols, const bool illegal)
{
	if (illegal) {
		BOOST_FOREACH (const char c, symbols.CStr()) {
			if (token.FindFirstOf(c) != String::NPos)
				return false;
		}
	} else {
		BOOST_FOREACH (const char c, token.CStr()) {
			if (symbols.FindFirstOf(c) == String::NPos)
				return false;
		}
	}

	return true;
}

static void HexEncode(char ch, std::ostream& os)
{
	const char *hex_chars = "0123456789ABCDEF";

	os << hex_chars[ch >> 4 & 0x0f];
	os << hex_chars[ch & 0x0f];
}

String Url::PercentDecode(const String& token)
{
	return Utility::UnescapeString(token);
}

String Url::PercentEncode(const String& token)
{
	std::ostringstream result;

	BOOST_FOREACH (const char c, token) {
		if (m_UnreservedCharacters.FindFirstOf(c) == String::NPos) {
			result << '%';
			HexEncode(c, result);
		} else
			result << c;
	}

	return result.str();
}


