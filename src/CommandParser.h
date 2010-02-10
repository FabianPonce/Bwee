#ifndef _COMMAND_PARSER_H
#define _COMMAND_PARSER_H
#include "StdAfx.h"

// define class prototypes so we can reference them in the typedefs before they exist
class Command;
class IRCSession;

// typedef for the method that returns a Command* with the appropriate settings. basically calls new Command() for every command.
typedef Command*(*CommandCreate)(IRCSession* pSession, string target, string sender, string text);

struct CommandProto
{
	string commandName;
	CommandCreate createFunc;

	std::map<std::string, CommandProto*> m_subCommandMap;
};

// Lookup map for CommandParser class
typedef map<string, CommandProto*> CommandCreateMap;

/*
 * Basic underlying Command class. This class will never be instantiated, but it providies the frameworks necessary
 * for higher-level commands to exist.
 */
class Command
{
protected:
	IRCSession* m_session;
	string m_target;
	string m_sender;
	WordStringReader m_reader;

public:
	Command(IRCSession* pSession, string target, string sender, string text);
	virtual ~Command() {};

	/*
	 * Checks the syntax of the given command and if necessary, preloads variables that run() will require.
	 * Returns true if the syntax is ok, false otherwise. In the event of false, getHelpText() will be called and displayed
	 * in the chat.
	 */
	virtual bool isSyntaxOk() = 0;

	/*
	 * Execute the command, doing it's actions. This method should be completely crash-safe assuming isSyntaxOk() passed.
	 */
	virtual void run() = 0;
	
	/*
	 * The help text that will be displayed in !help or if you misuse the command.
	 */
	virtual const char* getHelpText() { return DEFAULT_TEXT_HELP_NOT_AVAILABLE; }

	/*
	 * Returns the IRCSession pointer that this Command is executing on.
	 */
	IRCSession* GetSession() { return m_session; }
};

class CommandParser
{
private:
	IRCSession* m_session;

public:
	CommandParser(IRCSession* pSession);
	~CommandParser();

	/*
	 * Called to register all the core-defined commands into the CommandParser's list.
	 */
	static void registerCommands();

	/* 
	 * Registers a command with CommandParser, given a CommandProto pointer.
	 */
	static void registerCommand(CommandProto* cp);

	/*
	 * Attempts to execute a given command. If the command is unsuccessfully run, it's help text will be sent.
	 */
	void executeCommand(string target, string sender, string text);
	bool recursiveExecuteCommand(CommandProto* pBaseCmd, string target, string sender, string text);

	/*
	 * Sends the help text for Command c to the specified target.
	 */
	void sendHelpTextForCommand(Command* c, string target);

	/*
	 * Sends the help text for given commandName to specified target.
	 * commandName can be nonexistant, and will return a command does not exist error.
	 */
	void sendHelpTextForCommand(string commandName, string target);

	/*
	 * Builds and returns a comma-separated list of commands that are registered with the CommandParser.
	 */
	string buildCommandList();
	string buildCommandList(std::string commandName);

	/*
	 *
	 */
	static CommandProto* recursiveGetCommandProto(CommandProto* pProto, std::string cmdName);
	static CommandProto* getCommandProto(std::string cmdName);
};

#endif