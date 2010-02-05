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
using namespace std;
CLog Log;
int main()
{
	printf("Bwee IRC Bot\n");
	printf("By Valroft of http://www.mintwow.com/\n");
	printf("This program is licensed under the GNU Affero GPL.\n");
	printf("-------------------------------------------------------\n");
	Log.Notice("Bwee", "Starting up...");

#ifdef WIN32
	// WSA Setup
	WSADATA info;
	WSAStartup(MAKEWORD(2,0), &info);
#endif

	new IRCSession(BWEE_CONFIGURATION_FILE);
	// Keep the main thread busy. Doo doo doooooo.
	for(;;)
	{
		Sleep(1000); 
	}
	return 0;
}

