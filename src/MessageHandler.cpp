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

void IRCSession::HandleErrNotRegistered(IRCMessage& recvData)
{
	SendIdentification();
}

void IRCSession::HandleSuccessfulAuth(IRCMessage& recvData)
{
	// We're good to go.
	mConnState = CONN_REGISTERED;
	Log.Notice("IRCSession", "Authentication successful.");
	mServerRealName = recvData.hostmask;

	// Nickserv identification
	if(mUseNickServ)
	{
		Log.Notice("NickServ", "Sending NickServ identification.");
		SendChatMessage( PRIVMSG, "NickServ", "identify %s", mNickServPassword.c_str());
	}

	// Join the default channels
	std::map<string,string>::iterator itr = mChannelList.begin();
	for(; itr != mChannelList.end(); itr++)
	{
		string join = itr->first;
		if(itr->second != "")
			join += " " + itr->second;

		WriteLine("JOIN %s", join.c_str());
	}
}

void IRCSession::HandleMotdStart(IRCMessage& recvData)
{
	// The MOTD is coming 
	mHasFullMotd = false;
	mLastMotd = "";
}

void IRCSession::HandleMotd(IRCMessage& recvData)
{
	// Part of the MOTD, append it to the rest.
	if(mHasFullMotd)
		return;

	mLastMotd += recvData.args + NEWLINE;
}

void IRCSession::HandleMotdStop(IRCMessage& recvData)
{
	// The MOTD is over.
	Log.Notice("IRCSession", "Server Message of the Day received.");
	mHasFullMotd = true;
}

void IRCSession::HandleNotice(IRCMessage& recvData)
{
	Log.Color(TRED);
	printf("%s", recvData.source_nick.c_str());
	Log.Color(TYELLOW);
	printf(" sends notice: ");
	Log.Color(TNORMAL);
	printf("%s%s", recvData.args.c_str(), NEWLINE);
}

void IRCSession::HandlePrivmsg(IRCMessage& recvData)
{
	Log.Color(TYELLOW);
	printf("[%s] <%s> %s%s", recvData.target.c_str(), recvData.source_nick.c_str(), recvData.args.c_str(), NEWLINE);
	Log.Color(TNORMAL);

	// It's a command!
	if(recvData.args.substr(0, 1) != "!")
		return;

	if(mAntiSpamTicker + 2500 > GetTickCount())
		return;

	mAntiSpamTicker = GetTickCount();

	uint32 firstSpace = (uint32)recvData.args.find(' ');
	if(firstSpace == 0)
		firstSpace = (uint32)recvData.args.length();

	uint32 secondSpace = 0;
	if( firstSpace > 0 )
	{
		secondSpace = (uint32)recvData.args.find(' ', firstSpace+1);
	}

	string cmd = string(strlwr((char*)recvData.args.substr(1, firstSpace - 1).c_str()));
	if(cmd == "info")
	{
		// Really? Stop looking here. If you're going to edit this, at least don't steal credit.
		//  - Valroft
		SendChatMessage(PRIVMSG, recvData.target.c_str(), "I am %s, an IRC bot programmed by Valroft of MintWoW to integrate with MaNGOS-based servers.", mNickName.c_str());
		return;
	}

	if(cmd == "online")
	{
		if(recvData.args.length() <= firstSpace+1)
		{
			SendChatMessage(PRIVMSG, recvData.target.c_str(), "Incorrect format. Expected format is !online $realmname");
			return;
		}

		string realmName = recvData.args.substr(firstSpace+1);
		MySQLConnection* mSQLConn = GetRealm(GetRealmID(realmName.c_str()))->GetDB();
		QueryResult * query = mSQLConn->Query("SELECT COUNT(*) FROM characters WHERE online > 0");
		if(query)
		{
			uint32 result = query->Fetch()[0].GetUInt32();
			SendChatMessage(PRIVMSG, recvData.target.c_str(), "There are currently %u players online.", result);
			delete query;
		}
		query = mSQLConn->Query("SELECT COUNT(*) FROM characters WHERE online > 0 AND race IN (1,3,4,7,11)");
		if(query)
		{
			uint32 result = query->Fetch()[0].GetUInt32();
			SendChatMessage(PRIVMSG, recvData.target.c_str(), "There are currently %u Alliance players online.", result);
			delete query;
		}
		query = mSQLConn->Query("SELECT COUNT(*) FROM characters WHERE online > 0 AND race IN (2,5,6,8,10)");
		if(query)
		{
			uint32 result = query->Fetch()[0].GetUInt32();
			SendChatMessage(PRIVMSG, recvData.target.c_str(), "There are currently %u Horde players online.", result);
			delete query;
		}
		return;
	}

	if(cmd == "playerinfo")
	{
		if(recvData.args.length() <= secondSpace+1)
			return;

		const char* realm = recvData.args.substr(firstSpace+1,secondSpace-firstSpace+1).c_str();
		MySQLConnection* mSQLConn = GetRealm(GetRealmID(realm))->GetDB();
		string player = mSQLConn->EscapeString(recvData.args.substr(secondSpace+1));
		QueryResult * result = mSQLConn->Query("SELECT level,race,class,totalKills,gender,online FROM characters WHERE name = '%s'", player.c_str());
		if(result)
		{
			do 
			{
				string name = player;
				uint32 level = result->Fetch()[0].GetUInt32();
				uint32 raceid = result->Fetch()[1].GetUInt32();
				uint32 classid = result->Fetch()[2].GetUInt32();
				uint32 kills = result->Fetch()[3].GetUInt32();
				uint32 gender = result->Fetch()[4].GetUInt32();
				bool online = (result->Fetch()[5].GetUInt32() > 0);

				string races[] = { "", "human", "orc", "dwarf", "night elf", "undead", "tauren", "gnome", "troll", "", "blood elf", "draenei"};
				string classes[] = { "", "warrior", "paladin", "hunter", "rogue", "priest", "death knight", "shaman", "mage", "warlock", "", "druid" };

				SendChatMessage(PRIVMSG, recvData.target.c_str(), "%s is a level %u %s %s with %u honorable kills. %s is currently %s.", name.c_str(), level, races[raceid].c_str(), classes[classid].c_str(), kills, name.c_str(), online ? "online" : "offline");
			} while(result->NextRow());
			delete result;
		}
		else
		{
			SendChatMessage(PRIVMSG, recvData.target.c_str(), "I could find no character with that name.");
		}
	}
	if(cmd == "roll")
	{
		uint32 result = mRandGenerator->randInt(100);
		SendChatMessage(PRIVMSG, recvData.target.c_str(), "You roll a %u!", result);
	}
}

void IRCSession::HandlePing(IRCMessage& recvData)
{
	// Ping? Pong!
	WriteLine("PONG :%s", recvData.args.c_str());
	Log.Notice("IRCSession", "Ping? Pong!");
}

void IRCSession::HandlePong(IRCMessage& recvData)
{
	// Ping? Pong!
	WriteLine("PONG :%s", recvData.args.c_str());
	Log.Notice("IRCSession", "Ping? Pong!");
}

void IRCSession::HandleKick(IRCMessage& recvData)
{
	string kickedby = recvData.source_nick;
	string channel = recvData.argv[0];
	string kicked = recvData.argv[1];
	string reason = "";
	if(recvData.argc >= 3)
		reason = recvData.argv[2];

	// Were we kicked? Attempt to rejoin
	if(kicked == mNickName)
	{
		WriteLineForce("JOIN %s", recvData.target.c_str());
		Log.Color(TRED);
		printf("%s kicked me from %s for reason: \"%s\"\n", kickedby.c_str(), channel.c_str(), reason.c_str());
		Log.Color(TNORMAL);
	}
	else
	{
		printf("%s was kicked from %s by %s for \"%s\"\n", kicked.c_str(), channel.c_str(), kickedby.c_str(), reason.c_str());
	}
}

void IRCSession::HandleNick(IRCMessage& recvData)
{
	Log.Color(TRED);
	printf("%s", recvData.source_nick.c_str());
	Log.Color(TWHITE);
	printf(" changed his nick to: ");
	Log.Color(TRED);
	printf("%s\n", recvData.args.c_str());
	Log.Color(TNORMAL);
}