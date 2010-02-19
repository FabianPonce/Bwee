/*
* Bwee IRC Bot
* Copyright (C) 2010 Valroft <http://www.mintwow.com/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "StdAfx.h"

MySQLConnection::MySQLConnection(string host, int port, string user, string password)
{
	Guard g(mMutex);
	handle = mysql_init( NULL );
	mysql_real_connect(handle, host.c_str(), user.c_str(), password.c_str(), "", port, "", 0);
	if(!handle)
	{
		Log.Notice("mySQL", "Error connecting to database on %s", host.c_str());
		return;
	}
	mPort = port;
	mHost = host;
	mUser = user;
	mPassword = password;
}

MySQLConnection::~MySQLConnection()
{
	if(handle)
		mysql_close(handle);
}

void MySQLConnection::Execute(string query)
{
	Guard g(mMutex);
	if(!handle)
		_reconnect();

	int result = mysql_query(handle, query.c_str());

	if(result > 0)
	{
		uint32 errnom = mysql_errno(handle);
		const char * reason = mysql_error(handle);
		Log.Notice("MySQL", "Query Failed: %s\nReason: %s", query.c_str(), reason);

		if( errnom == 2006 || errnom == 2008 || errnom == 2013 || errnom == 2055 )
		{
			_reconnect();
			Execute(query);
		}
	}
}

QueryResult MySQLConnection::Query(const char * query, ...)
{
	char sql[16384];
	va_list vlist;
	va_start(vlist, query);
	vsnprintf(sql, 16384, query, vlist);
	va_end(vlist);

	string querystring = string(sql);
	Guard g(mMutex);
	if(!handle)
		_reconnect();

	int result = mysql_query(handle, querystring.c_str());

	if(result > 0)
	{
		uint32 errnom = mysql_errno(handle);
		const char * reason = mysql_error(handle);
		Log.Notice("MySQL", "Query Failed: %s\nReason: %s", querystring.c_str(), reason);

		if( errnom == 2006 || errnom == 2008 || errnom == 2013 || errnom == 2055 )
		{
			_reconnect();
			Query(querystring.c_str());
		}
	}

	MYSQL_RES * pRes = mysql_store_result( handle );
	uint32 uRows = (uint32)mysql_affected_rows( handle );
	uint32 uFields = (uint32)mysql_field_count( handle );

	if( uRows == 0 || uFields == 0 || pRes == 0 )
	{
		if( pRes != NULL )
			mysql_free_result( pRes );

		return QueryResult();
	}

	QueryResult res;
	res = QueryResult(new _QueryResult(pRes, uFields, uRows));
	res->NextRow();

	return res;
}

string MySQLConnection::EscapeString(string Escape)
{
	char a2[16384] = {0};
	const char * ret;
	if(mysql_real_escape_string(handle, a2, Escape.c_str(), (unsigned long)Escape.length()) == 0)
		ret = Escape.c_str();
	else
		ret = a2;

	return string(ret);
}

bool MySQLConnection::UseDatabase(string database)
{
	Guard g(mMutex);
	mDatabase = database;
	return mysql_select_db(handle, database.c_str()) <= 0;
}

void MySQLConnection::_reconnect()
{
	for(;;)
	{
		if(handle)
			mysql_close( handle );
	
		handle = mysql_init(NULL);
		mysql_real_connect(handle, mHost.c_str(), mUser.c_str(), mPassword.c_str(), mDatabase.c_str(), mPort, NULL, 0);
		if( handle == NULL )
		{
			Log.Error("mySQL", "Unable to reconnect to mySQL server!");
			continue;
		}

		if( mysql_select_db(handle, mDatabase.c_str()) > 0 )
		{
			Log.Error("mySQL", "Unable to select database %s.", mDatabase.c_str());
			continue;
		}

		Log.Notice("mySQL", "Reconnected successfully.");

		break;
	}
}