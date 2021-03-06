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

Mutex mCreateLock;

void IRCSession::InitializeHandlers()
{
	mCreateLock.Acquire();
	if( !IRCMessageHandlerMap.empty() )
	{
		mCreateLock.Release();
		return;
	}

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
	ADD_CODE( RPL_ERR_NICKNAMETAKEN, &IRCSession::HandleErrNickNameTaken );

	mCreateLock.Release();
}

IRCSession::IRCSession(string config) : ThreadContext(), mUpdateTimer(IRCSESSION_UPDATE_INTERVAL)
{
	// prevent Bwee from shutting down while any connections are active.
	sBweeStopEvent.mark();

	InitializeHandlers();

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

	mCmdParser = new CommandParser(this);
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

	string server = GetConfig()->GetStringDefault("IRC", "Server", "");
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
			conn->UseDatabase(database);
			m_realms[i] = new Realm( name, conn );
			m_realmMap[ m_realms[i]->GetName() ] = i;
		}
	}
	else
		m_realms = NULL;

}

IRCSession::~IRCSession()
{
	// cease all updates!
	sThreadPool.AbortTask(this);

	if(mSocket)
	{
		mSocket->Disconnect();
		delete mSocket;
	}

	delete mCmdParser;
	delete mRandGenerator;
	delete mConfigFile;

	for( uint32 i = 0; i < GetRealmCount(); ++i )
	{
		delete m_realms[i]->GetDB();
		delete m_realms[i];
	}
	delete [] m_realms;

	// tell bwee it can shutdown if no more sessions are running.
	sBweeStopEvent.unmark();
}

void IRCSession::OnRecv(string recvString)
{
	// HACK: PING (FIXME)
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

	// hokay, now argc/argv based on WordStringReader params ;D
	{
		string workStr = recvString.substr( recvString.find(' ') + 1 );
		WordStringReader wsr(workStr);

		vector<string> words;
		while( wsr.hasNextWord() )
		{
			string word = wsr.getNextWord();
			if( word.find(':') != string::npos )
			{
				// remainder-ize!
				size_t t = word.find(':');
				words.push_back( word.substr(0, t-1) );
				string post = word.substr(t+1);
				post += wsr.getRemainder();
				words.push_back(post);
				break;
			}
			
			words.push_back(word);
		}

		// import words!
		mess.argv = new string[ words.size() ];
		uint32 x = 0;
		for(vector<string>::iterator itr = words.begin(); itr != words.end(); ++itr)
		{
			mess.argv[x] = (*itr);
			++x;
		}
		mess.argc = (uint32)words.size();
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
		delete [] mess.argv;
		return;
	}

	// Pass this on to the correct handler.
	IRCCallback cb = itr->second;
	(this->*cb)(mess);

	delete [] mess.argv;
}

void IRCSession::Shutdown()
{
	mConnState = CONN_QUITTING;
	WriteLine("QUIT :%s", "Bwee IRC Bot %s - %s", PLATFORM_TEXT, CONFIG);
}

void IRCSession::Update()
{
	mUpdateTimer.mark();
	if( !mUpdateTimer.met() )
		return;

	if( mConnState == CONN_QUITTING )
	{
		delete this;
		return;
	}

	if(!mSocket->IsConnected())
	{
		//mSocket->WipeBuffers();
		mSocket->Connect(mHost, mPort);
		Log.Notice("IRCSession", "Lost connection to %s, reconnecting...", mHost.c_str());
		mConnState = CONN_CONNECTED;
		Sleep(20);
		return;
	}

	if(mConnState == CONN_CONNECTED)
	{
		SendIdentification();
		mConnState = CONN_REGISTERING;
	}

	if( mNickNameRetry )
	{
		mNickNameRetry = 0;
		SendIdentification();
	}

	while(mSocket->HasLine())
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

Realm* IRCSession::GetRealm(string n)
{
	map<string, uint32>::iterator itr = m_realmMap.begin();
	for(; itr != m_realmMap.end(); ++itr)
	{
		string s = itr->first;
		uint32 i = itr->second;

		uint32 r = strnicmp(s.c_str(), n.c_str(), s.length());
		if( r == 0 )
			return GetRealm(itr->second);
	}

	return NULL;
}