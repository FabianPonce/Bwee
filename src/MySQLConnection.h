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

#ifndef _MYSQL_CONNECTION_H
#define _MYSQL_CONNECTION_H

#include "StdAfx.h"

typedef void(*mySQLCallback)(QueryResult);
struct ASyncQuery
{
	mySQLCallback callback;
	string query;
};

class MySQLConnection
{
public:
	MySQLConnection(string host, int port, string user, string password);
	~MySQLConnection();

	bool UseDatabase(string database);
	void Execute(string query);
	QueryResult Query(const char * query, ...);

	string EscapeString(string Escape);

protected:
	void _reconnect();

	Mutex mMutex;
	MYSQL* handle;

	int mPort;
	string mHost;
	string mUser;
	string mPassword;
	string mDatabase;
};

#endif