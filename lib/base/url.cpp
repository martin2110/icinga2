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
#include "base/url.tcpp"
#include "url-characters.hpp"
#include "base/array.hpp"
#include "base/utility.hpp"
#include "base/primitivetype.hpp"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>

using namespace icinga;

REGISTER_TYPE(Url);

bool Url::Parse(const String& base_url)
{
	m_Valid = false;
	String url = base_url;

	if (url.GetLength() == 0)
		return m_Valid;

	size_t pHelper = url.Find(":");

	if (pHelper == String::NPos) {
		m_Scheme = "";
	} else {
		if (!ParseScheme(url.SubStr(0, pHelper)))
			return m_Valid;
		url = url.SubStr(pHelper+1);
	}

	if (*url.Begin() != '/')
		return m_Valid;
	
	if (url.GetLength() == 1) {
		m_Path.push_back("/");
		m_Valid = true;
		return m_Valid;
	}

	if (*(url.Begin()+1) != '/')
		m_Authority = "";
	else {
		pHelper = url.Find("/", 2);
		if (pHelper == String::NPos)
			return m_Valid;
		if (!ParseAuthority(url.SubStr(2, pHelper-2)))
			return m_Valid;
		url = url.SubStr(pHelper);
	}
/*
	if (!m_Scheme.IsEmpty() && m_Authority.IsEmpty())
		return m_Valid;
*/

	pHelper = url.FindFirstOf("#?");

	if (!ParsePath(url.SubStr(1, pHelper-1)))
		return m_Valid;

	url = url.SubStr(pHelper);

	if (*url.Begin() == '?') {
		pHelper = url.Find("#");
		if (!ParseQuery(url.SubStr(0, pHelper)))
			return m_Valid;
	}

	m_Valid = true;

	return m_Valid;
}

String Url::GetAuthority(void) const
{
	return m_Authority;
}

String Url::GetScheme(void) const
{
	return m_Scheme;
}

std::map<String,Value> Url::GetQuery(void) const
{
	return m_Query;
}

Value Url::GetQueryElement(const String& name) const
{
	std::map<String, Value>::const_iterator it = m_Query.find(name);

	if (it == m_Query.end())
		return Empty;

	return it->second;
}

std::vector<String> Url::GetPath(void) const
{
	return m_Path;
}

bool Url::IsValid(void) const
{
	return m_Valid;
}

String Url::Format(void) const
{
	if (!IsValid())
		return "";

	String url = "";

	if (!m_Scheme.IsEmpty())
		url += m_Scheme + "://";

	url += m_Authority;

	BOOST_FOREACH (const String p, m_Path) {
		url += '/';
		url += Utility::EscapeString(p, ACPATHSEGMENT, false);
	}

	String param = "";

	if (!m_Query.empty()) {
		typedef std::pair<String,Value> kv_pair;

		BOOST_FOREACH (const kv_pair kv, m_Query) {
			String key = Utility::EscapeString(kv.first, ACQUERY, false);

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
					sArr += key + "[]=" + Utility::EscapeString(sArrIn, ACQUERY, false);
				}
			} else
				param += key + "=" + Utility::EscapeString(kv.second, ACQUERY, false);
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

	return (ValidateToken(ischeme, ACSCHEME, false));
}

bool Url::ParseAuthority(const String& authority)
{
	//TODO parse all Authorities
	m_Authority = authority;

	return (ValidateToken(authority, ACHOST, false));
}

bool Url::ParsePath(const String& path)
{

	std::string pathStr = path;
	boost::char_separator<char> sep("/");
	boost::tokenizer<boost::char_separator<char> >
	    tokens(pathStr, sep);
	String decodedToken;

	BOOST_FOREACH(const String& token, tokens) {
		decodedToken = Utility::UnescapeString(token);

		if (!ValidateToken(decodedToken, ACPATHSEGMENT, false))
			return false;

		m_Path.push_back(decodedToken);
		//TODO Validate + deal with .. and .
	}

	return true;
}

bool Url::ParseQuery(const String& parameters)
{
	//Tokenizer does not like String AT ALL
	std::string parametersStr = parameters;
	boost::char_separator<char> sep("&");
	boost::tokenizer<boost::char_separator<char> >
	    tokens(parametersStr, sep);


	BOOST_FOREACH(const String& token, tokens) {
		size_t kvSep = token.Find("=");

		std::cout << token << '\n';
		if (kvSep == String::NPos)
			return false;

		String key = token.SubStr(0, kvSep);
		String value = token.SubStr(kvSep+1);
		
		if (key.IsEmpty() || value.IsEmpty())
			return false;

		if (key.GetLength() >= 3 && *key.RBegin() == ']' && *(key.RBegin()+1) == '[') {
			key = key.SubStr(0, key.GetLength() - 2);
			key = Utility::UnescapeString(key);
			std::map<String, Value>::iterator it = m_Query.find(key);

			if (it == m_Query.end()) {
				Array::Ptr tmp = new Array();
				tmp->Add(Utility::UnescapeString(value));
				m_Query[key] = tmp;
			} else if (m_Query[key].IsObjectType<Array>()){
				Array::Ptr arr = it->second;
				arr->Add(Utility::UnescapeString(value));
			} else
				return false;
		} else {
			key = Utility::UnescapeString(key);
			if (m_Query.find(key) == m_Query.end())
				m_Query[key] = Utility::UnescapeString(value);
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
