#ifndef _COMMAND_PARSER_H
#define _COMMAND_PARSER_H
#include "StdAfx.h"

class Command;
class IRCSession;
typedef Command*(*CommandCreate)(IRCSession* pSession, string target, string sender, string text);

struct CommandProto
{
	string commandName;
	CommandCreate createFunc;
};

typedef map<string, CommandProto*> CommandCreateMap;

class Command
{
protected:
	IRCSession* m_session;
	string m_target;
	string m_sender;
	string m_text;
	size_t m_readPos;

public:
	Command(IRCSession* pSession, string target, string sender, string text);
	virtual ~Command() {};

	virtual bool isSyntaxOk() = 0;
	virtual void run() = 0;
	
	virtual const char* getHelpText() { return "Help for this command is not available."; }

	bool hasNextWord();
	string getNextWord();
	string getRemainder();
	IRCSession* GetSession() { return m_session; }
};

class CommandParser
{
private:
	IRCSession* m_session;

public:
	CommandParser(IRCSession* pSession);
	~CommandParser();

	static void registerCommands();
	static void registerCommand(CommandProto* cp);

	void executeCommand(string target, string sender, string text);
	void sendHelpTextForCommand(Command* c, string target);
	void sendHelpTextForCommand(string commandName, string target);

	string buildCommandList();
};

#endif