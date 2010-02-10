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
	if(recvData.args[0] == '!')
	{
		GetCommandParser()->executeCommand(recvData.target, recvData.source_user, recvData.args);
		return;
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
		WriteLine("JOIN %s", recvData.target.c_str());
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