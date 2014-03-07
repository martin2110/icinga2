/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#include "base/statsfunction.h"
#include "base/registry.h"
#include "base/singleton.h"

using namespace icinga;

StatsFunction::StatsFunction(const Callback& function)
	: m_Callback(function)
{ }

Value StatsFunction::Invoke(Dictionary::Ptr& status, Dictionary::Ptr& perfdata)
{
	return m_Callback(status, perfdata);
}

RegisterStatsFunctionHelper::RegisterStatsFunctionHelper(const String& name, const StatsFunction::Callback& function)
{
	StatsFunction::Ptr func = make_shared<StatsFunction>(function);
	StatsFunctionRegistry::GetInstance()->Register(name, func);
}

StatsFunctionRegistry *StatsFunctionRegistry::GetInstance(void)
{
	return Singleton<StatsFunctionRegistry>::GetInstance();
}
