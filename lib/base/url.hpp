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

#ifndef URL_H
#define URL_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/string.hpp"
#include "base/value.hpp"
#include <map>
#include <vector>

namespace icinga
{
/**
 * A url parser to use with the API
 *
 * @ingroup base
 */

class I2_BASE_API Url
{
public:
	Url(const String& url);
	
	String Format(void);
	bool IsValid(void);
	void Initialize(void);

	String GetHost(void);
	String GetScheme(void);
	std::map<String,Value> GetParameters(void);
	Value GetParameter(const String& name);
	std::vector<String> GetPath(void);

private:
	bool m_Valid;
	String m_UnreservedCharacters;

	String m_Host;
	String m_Scheme;
	std::map<String,Value> m_Parameters;
	std::vector<String> m_Path;

	bool ValidateToken(const String& token, const String& symbols, const bool illegal=0);
	bool ParseScheme(const String& scheme);
	bool ParseHost(const String& host);
	bool ParsePath(const String& path);
	bool ParseParameters(const String& path);

	String PercentDecode(const String& token);
	String PercentEncode(const String& token);
};

}
#endif /* URL_H */
