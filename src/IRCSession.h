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

#ifndef _IRC_SESSION_H
#define _IRC_SESSION_H

#include "StdAfx.h"

#define CONN_CONNECTED 0
#define CONN_REGISTERING 1
#define CONN_REGISTERED 2

class MySQLConnection;
class SimpleSocket;

enum MessageType
{
	PRIVMSG,
	NOTICE,
};

struct IRCMessage
{
	string hostmask;
	string opcode;
	string target; // optional
	string args; 

	// these two are derived; :bob!jim@gym
	string source_nick; // bob
	string source_user; // jim
	string source_host; // gym

	// The number of argvs
	uint32 argc;

	/* Format for argv:
	 * argv is the space-delimited array of parameters sent from the 
	 * IRCd. This should be used when the standard parser fails.
	 * In cases where we run into a colon parameter, the colon is removed
	 * and everything after it appears as one argument.
	 * The argv begins recording AFTER the IRC RPL code.
	 */
	string *argv;
};

class IRCSession : public IRunnable
{
public:
	IRCSession(std::string config);
	~IRCSession();

	/*
	 * Rehashes the Bwee.conf configuration file.
	 */
	void RehashConfig();

	/*
	 * Outputs a message to the server. You need not include \n.
	 */
	void WriteLine(const char * format, ...);
	void WriteLineForce(const char * format, ...);

	/*
	 * Callback for SimpleSocket I/O. Do not invoke directly.
	 */
	void OnRecv(string recvString);

	/*
	 * Send a simple chat message. 
	 * @param MessageType Either "PRIVMSG" or "NOTICE" now.
	 * @param target The target of the message. Can be a channel or a user.
	 * @param format The message to send. May include formatters such as %s, %u, etc.
	 */
	void SendChatMessage(MessageType type, const char * target, const char * format, ...);

	/*
	 * Updates the IRC Session. Do not invoke directly.
	 */
	void Update();

	Realm* GetRealm(uint32 id) { return m_realms[id]; }
	uint32 GetRealmID(const char* n) { return m_realmMap[n]; }

	/*
	 * Sends NICK and USER responses to the server.
	 */
	void SendIdentification();

	/*
	 * Returns the configuration controller
	 */
	ConfigFile* GetConfig() { return mConfigFile; }

protected:
	/* Message Handlers
	* --------------------------------
	* Invoked automatically by OnRecv. Do not invoke directly.
	*/
	void HandleSuccessfulAuth(IRCMessage& recvData);
	void HandleMotdStart(IRCMessage& recvData);
	void HandleMotd(IRCMessage& recvData);
	void HandleMotdStop(IRCMessage& recvData);
	void HandlePrivmsg(IRCMessage& recvData);
	void HandleNotice(IRCMessage& recvData);
	void HandlePing(IRCMessage& recvData);
	void HandlePong(IRCMessage& recvData);
	void HandleKick(IRCMessage& recvData);
	void HandleNick(IRCMessage& recvData);
	void HandleErrNotRegistered(IRCMessage& recvData);
 
	// the config file!
	string mConfig;

	// The current host we're connected to as specified in the config file.
	string mHost;
	// The current port we're connected to as specified in the config file.
	uint32 mPort;
	// The Socket ptr.
	SimpleSocket* mSocket;
	// Connection state: either CONN_CONNECTED, CONN_REGISTERING or CONN_REGISTERED
	uint32 mConnState;
	// The time of the last ping sent to the server.
	uint32 mLastPing;

	string mUserName;
	string mNickName;
	string mHostName;
	string mServerName;
	string mRealName;

	// Nickserv Module
	bool mUseNickServ;
	string mNickServPassword;

	// The name the server refers to itself as.
	string mServerRealName;

	// The last received MOTD. Be sure to check whether mHasFullMotd is true before using this.
	string mLastMotd;
	// Whether or not the mLastMotd variable is populated fully.
	bool mHasFullMotd;

	// A list of channels and their passwords.
	std::map<string,string> mChannelList;

	// Pointer to our realms.
	Realm **  m_realms;
	std::map<const char*, uint32> m_realmMap;

	// Time until we can accept a new privmsg "command"
	uint32 mAntiSpamTicker;

	// The random number generator
	MTRand * mRandGenerator;

	// Configuration File class
	ConfigFile* mConfigFile;

	// The thread we're running on
	Thread * mThread;
};
typedef void(IRCSession::*IRCCallback)(IRCMessage& recvData);
typedef std::map<std::string, IRCCallback> MessageHandlerMap;
extern MessageHandlerMap IRCMessageHandlerMap;
#endif