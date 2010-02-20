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
CLog Log;

int main()
{
	Log.Notice("Bwee", "Bwee IRC Bot");
	Log.Notice("Bwee", "By Valroft of http://www.mintwow.com/");
	Log.Notice("Bwee", "This program is licensed under the GNU Affero GPL.");
	Log.Notice("Bwee", "-------------------------------------------------------");
	Log.Notice("Bwee", "Starting up...");
	
	new BweeGlobalStopEvent;
	new ThreadPool;

	sThreadPool.ExecuteTask( new IRCSession(BWEE_CONFIGURATION_FILE) );

	// Keep the main thread busy until all IRCSessions are despawned.
	while( sBweeStopEvent.marked() )
	{
		Sleep(1000); 
	}
	return 0;
}

