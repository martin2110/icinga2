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
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_url)

BOOST_AUTO_TEST_CASE(construct)
{
	Url::Url *url = new Url("");
	BOOST_CHECK(url);
}

BOOST_AUTO_TEST_CASE(url_id_and_path)
{
	Url::Url *url = new Url("http://icinga.org/foo/bar/baz?hurr=durr");
	BOOST_CHECK(url->GetScheme() == "http");
	BOOST_CHECK(url->GetHost() == "icinga.org");
	std::vector<String> PathCorrect;
	PathCorrect.push_back("foo");
	PathCorrect.push_back("bar");
	PathCorrect.push_back("baz");
	BOOST_CHECK(url->GetPath() == PathCorrect);
}

BOOST_AUTO_TEST_CASE(url_parameters)
{
	Url::Url *url = new Url("https://icinga.org/hya/?rain=karl&rair=robert&foo[]=bar");

	BOOST_CHECK(url->GetParameter("rair") == "robert");
	BOOST_CHECK(url->GetParameter("rain") == "karl");
	BOOST_CHECK(url->GetParameter("foo").IsObjectType<Array>());
	Array::Ptr test = url->GetParameter("foo");
	BOOST_CHECK(test->GetLength() == 1);
	BOOST_CHECK(test->Get(0) == "bar");
}

BOOST_AUTO_TEST_CASE(url_format)
{
	String surl;
	Url::Url *url;

	surl = "http://foo.bar/baum";
    url = new Url(surl);
	BOOST_CHECK(surl == url->Format());

	surl = "http://foo.bar/%42aum/../Baum";
    url = new Url(surl);
	BOOST_CHECK("http://foo.bar/Baum" == url->Format());


}

BOOST_AUTO_TEST_CASE(url_illegal_legal_strings)
{
	Url::Url *url;
	url = new Url("/?foo=barr&foo[]=bazz");
	BOOST_CHECK(!url->IsValid());
	url = new Url("/?]=gar");
	BOOST_CHECK(!url->IsValid());
	url = new Url("/?foo=baz??&???");
	BOOST_CHECK(!url->IsValid());
	url = new Url("/?foo=bar&foo=baz");
	BOOST_CHECK(!url->IsValid());
	url = new Url("/?foo=bar&[]=d");
	BOOST_CHECK(!url->IsValid());
	url = new Url("/?fo=&bar=garOA");
	BOOST_CHECK(!url->IsValid());
}

BOOST_AUTO_TEST_SUITE_END()
