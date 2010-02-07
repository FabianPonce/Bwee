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

MessageHandlerMap IRCMessageHandlerMap;

#define ADD_CODE(code,method) \
	IRCMessageHandlerMap.insert( make_pair(code, method) );

IRCSession::IRCSession(std::string config) : IRunnable()
{
	mConfigFile = NULL;
	mConfig = config;
	mSocket = NULL;
	RehashConfig();
	if( !GetConfig() )
	{
		Log.Error("IRCSession", "Fatal error: cannot load config file %s.", mConfig.c_str());
		exit(-1);
		return;
	}

	mConnState = CONN_CONNECTED;

	mRandGenerator = new MTRand;

	mHasFullMotd = false;
	mLastPing = GetTickCount();

	// Populate the giant IRCSession handler table
	ADD_CODE( RPL_SUCCESSFUL_AUTH, &IRCSession::HandleSuccessfulAuth)
	ADD_CODE( RPL_MOTDSTART, &IRCSession::HandleMotdStart )
	ADD_CODE( RPL_MOTD, &IRCSession::HandleMotd )
	ADD_CODE( RPL_ENDOFMOTD, &IRCSession::HandleMotdStop )
	ADD_CODE( RPL_NOTICE, &IRCSession::HandleNotice )
	ADD_CODE( RPL_PRIVMSG, &IRCSession::HandlePrivmsg )
	ADD_CODE( RPL_PING, &IRCSession::HandlePing )
	ADD_CODE( RPL_PONG, &IRCSession::HandlePong )
	ADD_CODE( RPL_KICK, &IRCSession::HandleKick );
	ADD_CODE( RPL_NICK, &IRCSession::HandleNick );
	ADD_CODE( RPL_ERR_NOTREGISTERED, &IRCSession::HandleErrNotRegistered );

	mCmdParser = new CommandParser(this);

	mThread = new Thread(this);
	mThread->start();
}

void IRCSession::RehashConfig()
{
	if(!GetConfig())
	{
		mConfigFile = new ConfigFile();
		if( !mConfigFile->SetSource(mConfig.c_str()) )
		{
			Log.Notice("Config", "Error loading configuration file.");
			return;
		}
	}

	std::string server = GetConfig()->GetStringDefault("IRC", "Server", "");
	uint32 port = GetConfig()->GetIntDefault("IRC", "Port", 6667);
	if( !mSocket )
	{
		mSocket = new SimpleSocket();
		if(!mSocket->Connect(server, port))
		{
			Log.Notice("IRCSession", "Unable to open socket to %s.", server.c_str());
			exit(1);
			return;
		}
		Log.Notice("IRCSession", "Connected to %s successfully.", server.c_str());
		mHost = server;
		mPort = port;
	}

	mNickName = GetConfig()->GetStringDefault("User", "Nick", "Test");
	mUserName = GetConfig()->GetStringDefault("User", "Username", "Test");
	mHostName = GetConfig()->GetStringDefault("User", "Host", "bwee");
	mServerName = mHostName;

	mUseNickServ = GetConfig()->GetBoolDefault("NickServ", "Enable", false);
	mNickServPassword = GetConfig()->GetStringDefault("NickServ", "Password", "");

	uint32 channelCount = GetConfig()->GetIntDefault("Channels", "Count", 0);
	if(channelCount)
	{
		for(uint32 i = 1; i <= channelCount; i++)
		{
			char term[256];
			memset(term, '\0', 255);
			sprintf(term, "Channel%u", i);

			string config = string(term);
			string channel = GetConfig()->GetStringDefault(config.c_str(), "Name", "");
			if(channel == "")
			{
				Log.Notice("Config", "Error parsing channels configuration.");
				return;
			}

			string password = GetConfig()->GetStringDefault(config.c_str(), "Password", "");

			mChannelList.insert( make_pair(channel, password) );
		}
	}
	else
	{
		Log.Notice("Config", "Loaded 0 default channels.");
	}

	uint32 realmCount = GetConfig()->GetIntDefault("Realms", "Count", 0);
	if(realmCount)
	{
		m_realms = new Realm*[realmCount];
		for(uint32 i = 0; i < realmCount; ++i)
		{
			char term[256];
			memset(term, '\0', 255);
			sprintf(term, "Realm%u", i+1);

			string config = string(term);
			string name = GetConfig()->GetStringDefault(config.c_str(), "Name", "");
			if( name.length() == 0 )
			{
				Log.Error("Config", "Invalid realm configuration for realm %u", i+1);
				continue;
			}

			string dbhost = GetConfig()->GetStringDefault(config.c_str(), "DBHost", "localhost");
			string dbuser = GetConfig()->GetStringDefault(config.c_str(), "DBUser", "root");
			string dbpassword = GetConfig()->GetStringDefault(config.c_str(), "DBPassword", "");
			string database = GetConfig()->GetStringDefault(config.c_str(), "DBName", "characters");
			int dbport = GetConfig()->GetIntDefault(config.c_str(), "DBPort", 3306);
			MySQLConnection * conn = new MySQLConnection(dbhost, dbport, dbuser, dbpassword);
			conn->Query("USE %s", database.c_str());
			m_realms[i] = new Realm( name, conn );
			m_realmMap[ m_realms[i]->GetName() ] = i;
		}
	}
	else
		m_realms = NULL;

}

IRCSession::~IRCSession()
{
	delete mThread;

	if(mSocket)
		mSocket->Disconnect();

	delete mSocket;
	delete mCmdParser;
	delete mRandGenerator;
	delete mConfigFile;

	for( uint32 i = 0; i < GetRealmCount(); ++i )
	{
		delete m_realms[i]->GetDB();
		delete m_realms[i];
	}
	delete [] m_realms;
}

void IRCSession::OnRecv(string recvString)
{
	// HACK: PING
	if( recvString.substr(0,4).compare("PING") == 0 )
	{
		WriteLine("PONG :%s", recvString.substr(6).c_str());
		return;
	}

	IRCMessage mess;

	char hostmask[256];
	char opcode[256];
	char target[256];
	char args[4096];

	char source_nick[256];
	char source_host[256];

	// Make these just giant arrays of null strings.
	// This will allow for .c_str() to properly transfer the char arrays to the console
	memset(hostmask, '\0', 256);
	memset(opcode, '\0', 256);
	memset(target, '\0', 256);
	memset(args, '\0', 4096);
	memset(source_nick, '\0', 256);
	memset(source_host, '\0', 256);

	uint32 r = sscanf(recvString.c_str(), ":%255s %255s %255s :%4095[^\r\n]", hostmask, opcode, target, args);
	if(r != 4)
	{
		// Parsing failed, let's go to the fallback method. :P
		r = sscanf(recvString.c_str(), ":%255s %255s %4095[^\r\n]", hostmask, opcode, args);
	}

	// Fallback method, argc/argv
	mess.argc = 0;
	string *argv = new string[1024];
	uint32 lastSpace = 0;
	uint32 offset = (uint32)recvString.find(' ', recvString.find(' ')+1);
	uint32 newline = (uint32)recvString.find('\n');
	while(true)
	{
		uint32 spaceLoc = (uint32)recvString.find(' ', lastSpace+1);
		if(spaceLoc == string::npos)
			break;

		// Removing the first two arguments.
		if(spaceLoc <= offset)
		{
			lastSpace = spaceLoc;
			continue;
		}

		mess.argc++;

		argv[mess.argc - 1] = recvString.substr(lastSpace+1, spaceLoc-lastSpace-1);
		if(argv[mess.argc - 1].find(':') != string::npos)
		{
			// We have a colon, this basically means the rest of the text
			// is one giant argument. Remove the colon and go on.
			argv[mess.argc - 1] = recvString.substr(lastSpace+2);
			break;
		}

		lastSpace = spaceLoc;
	}
	// Lolcopy!
	mess.argv = new string[mess.argc];
	for(uint32 i = 0; i < mess.argc; i++)
	{
		if(i != mess.argc - 1)
			mess.argv[i] = argv[i];
		else
			mess.argv[i] = argv[i].substr(0, argv[i].length()-1);
	}

	mess.hostmask = string(hostmask);
	mess.opcode = string(opcode);
	mess.args = string(args);
	mess.target = string(target);

	// split the hostmask up into useful parts
	uint32 pos = (uint32)mess.hostmask.find('!');
	mess.source_nick = mess.hostmask.substr(0, pos);
	mess.source_host = mess.hostmask.substr(pos+1);

	pos = (uint32)mess.source_host.find('@');
	mess.source_user = mess.source_host.substr(0, pos);
	mess.source_host = mess.source_host.substr(pos+1);

	MessageHandlerMap::iterator itr = IRCMessageHandlerMap.find(mess.opcode);
	if( itr == IRCMessageHandlerMap.end() )
	{
		// Do not process this, print out some debug information
		Log.Notice("IRCSession", "Received unhandled opcode: %s", mess.opcode.c_str());
		return;
	}

	// Pass this on to the correct handler.
	IRCCallback cb = itr->second;
	(this->*cb)(mess);

	if(mess.argc)
		delete [] mess.argv;
}

void IRCSession::Update()
{
	while(true)
	{
		if( mConnState == CONN_QUITTING )
		{
			delete this;
			break;
		}

		if(!mSocket->IsConnected())
		{
			//mSocket->WipeBuffers();
			mSocket->Connect(mHost, mPort);
			Log.Notice("IRCSession", "Lost connection to %s, reconnecting...", mHost.c_str());
			mConnState = CONN_CONNECTED;
			Sleep(20);
			continue; // update next loop.
		}

		if(mConnState == CONN_CONNECTED)
		{
			SendIdentification();
			mConnState = CONN_REGISTERING;
		}

		if(mSocket->HasLine())
		{
			string recv = mSocket->GetLine();
			OnRecv(recv);
		}

		if(GetTickCount() - mLastPing > 15000)
		{
			WriteLine("PING :%s", mHost.c_str());
			mLastPing = GetTickCount();
		}

		mSocket->UpdateQueue();

		Sleep(20);
	}
}

void IRCSession::SendChatMessage(MessageType type, const char * target, const char * format, ...)
{
	char obuf[65536];
	va_list ap;

	va_start(ap, format);
	vsnprintf(obuf, 65536, format, ap);
	va_end(ap);

	if( *obuf == '\0' )
		return;
	
	string oss = "";
	if(type == PRIVMSG)
		oss = oss + "PRIVMSG";
	else if(type == NOTICE)
		oss = oss + "NOTICE";
	oss = oss + " "; 
	oss = oss + target + " :" + obuf + NEWLINE;
	WriteLine(oss.c_str());
}

void IRCSession::WriteLine(const char * format, ...)
{
	char obuf[65536];
	va_list ap;

	va_start(ap, format);
	vsnprintf(obuf, 65536, format, ap);
	va_end(ap);

	if( *obuf == '\0' )
		return;

	string send = string(obuf) + "\r\n";

	mSocket->SendLine(send);
	//mSendQueue.push_back(send);
}

void IRCSession::SendIdentification()
{
	WriteLine("NICK %s", mNickName.c_str());
	WriteLine("USER %s 8 * : %s", mNickName.c_str(), mNickName.c_str());
}

uint32 IRCSession::GetRealmID(std::string n)
{
	std::map<std::string, uint32>::iterator itr = m_realmMap.begin();
	for(; itr != m_realmMap.end(); ++itr)
	{
		string s = itr->first;
		uint32 i = itr->second;

		uint32 r = strnicmp(s.c_str(), n.c_str(), s.length());
		if( r == 0 )
			return itr->second;
	}

	return 0;
}