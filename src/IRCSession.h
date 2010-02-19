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
#define CONN_QUITTING 3

class MySQLConnection;
class SimpleSocket;
class CommandParser;

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
	/*
	 * Constructs a new IRCSession. 
	 * @param config The location of a valid Bwee configuration file.
	 */
	IRCSession(std::string config);
	~IRCSession();

	/*
	 * Rehashes the Bwee.conf configuration file. Also initializes sockets and database connections if necessary.
	 */
	void RehashConfig();

	/*
	 * Outputs a message to the server. Do not include linebreaks.
	 * This method supports printf-style formatting.
	 * @param format The format string or plain text to send.
	 */
	void WriteLine(const char * format, ...);

	/*
	 * Callback for SimpleSocket I/O. Do NOT invoke directly.
	 */
	void OnRecv(string recvString);

	/*
	 * Send a chat message over this IRC session.
	 * @param MessageType Either "PRIVMSG" or "NOTICE", depending on which type of chat message.
	 * @param target The target of the message. Can be a channel or a user.
	 * @param format The message to send. May include formatters such as %s, %u, etc.
	 */
	void SendChatMessage(MessageType type, const char * target, const char * format, ...);

	/*
	 * Updates the IRC Session. Do NOT invoke directly.
	 * This method runs in it's own thread and is called continuously.
	 */
	void Update();

	/* 
	 * Returns a realm by it's index/id 
	 * @param id The ID of the realm. This number MUST be valid or memory corruption will result.
	 */
	Realm* GetRealm(uint32 id) { return m_realms[id]; }
	
	/*
	 * Returns a realm by it's name. Names are not case sensitive.
	 * @param n The name of the realm (case insensitive).
	 */
	Realm* GetRealm(std::string n);

	/*
	 * Returns the number of registered realms.
	 */
	uint32 GetRealmCount() { return (uint32)m_realmMap.size(); }

	/*
	 * Returns the configuration controller
	 */
	ConfigFile* GetConfig() { return mConfigFile; }

	/*
	 * Returns the CommandParser that is being used by this IRCSession.
	 */
	CommandParser* GetCommandParser() { return mCmdParser; }

private:

	/*
	 * Initializes message handlers table which tells which IRC message leads to which method.
	 */
	static void InitializeHandlers();

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
	void HandleErrNickNameTaken(IRCMessage& recvData);

	/*
	* Sends NICK and USER responses to the server.
	*/
	void SendIdentification();
 
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
	std::map<std::string, uint32> m_realmMap;

	// The random number generator
	MTRand * mRandGenerator;

	// Configuration File class
	ConfigFile* mConfigFile;

	CommandParser* mCmdParser;

	// The thread we're running on
	Thread * mThread;

	// nickRetry
	uint32 mNickNameRetry;
};
typedef void(IRCSession::*IRCCallback)(IRCMessage& recvData);
typedef std::map<std::string, IRCCallback> MessageHandlerMap;
extern MessageHandlerMap IRCMessageHandlerMap;
#endif