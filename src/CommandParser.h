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

	map<string, CommandProto*> m_subCommandMap;
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
	 * Execute the command, doing it's actions. This method should be completely safe assuming isSyntaxOk() passed.
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

	/*
	 * Attempts to read a realm name from the reader and return a pointer to the corresponding Realm object.
	 */
	Realm* ReadRealm()
	{
		if( !m_reader.hasNextWord() )
			return NULL;

		return GetSession()->GetRealm( m_reader.getNextWord() );
	}
};

class CommandParser
{
private:
	IRCSession* m_session;

public:
	/*
	 * Constructs a new CommandParser.
	 * @param pSession The IRCSession commands will use to send chat and perform actions on.
	 */
	CommandParser(IRCSession* pSession);
	~CommandParser();

	/*
	 * Called to register all the core-defined commands into the CommandParser's list.
	 */
	static void registerCommands();

	/* 
	 * Registers a command with CommandParser, given a CommandProto pointer.
	 * @param cp A populated CommandProto that will be used to spawn instances of the command.
	 */
	static void registerCommand(CommandProto* cp);

	/*
	 * Attempts to execute a given command. If the command is unsuccessfully run, it's help text will be sent.
	 * @param target The place to send all messages involving this command to.
	 * @param sender The user who has invoked the command
	 * @param text The contents of the user's message, including the command, that will be parsed to find commands.
	 */
	void executeCommand(string target, string sender, string text);

	/*
	 * Attempts to locate and execute subcommands from a given base command. Should absolutely NOT be run
	 * from outside of CommandParser::executeCommand().
	 * @param pBaseCommand The command that will be searched for subcommands.
	 * @param target The place to send all messages involving this command to.
	 * @param sender The user who has invoked the command
	 * @param text The contents of the user's message, excluding the previous command, that will be searched.
	 */
	bool recursiveExecuteCommand(CommandProto* pBaseCmd, string target, string sender, string text);

	/*
	 * Sends the help text for Command c to the specified target.
	 * @param c The instance of a command to retrieve help text from.
	 * @param target The place to send the help text to.
	 */
	void sendHelpTextForCommand(Command* c, string target);

	/*
	 * Sends the help text for given commandName to specified target.
	 * commandName can be nonexistant, and will return a command does not exist error.
	 * @param commandName The name of the command to search for (supports recursion/subcommands)
	 * @param target The place that help text will be sent to.
	 */
	void sendHelpTextForCommand(string commandName, string target);

	/*
	 * Builds and returns a comma-separated list of commands that are registered with the CommandParser.
	 */
	string buildCommandList();

	/*
	 * Builds and returns a comma-separated list of commands that are registered as subcommands of the given command.
	 * @param commandName The command that is assumed to have subcommands, which will be listed for.
	 */
	string buildCommandList(string commandName);

	/*
	 * Returns the CommandProto of the specified command.
	 * @param cmdName The command to search for (supports recursion/subcommands)
	 */
	static CommandProto* getCommandProto(string cmdName);

	/*
	 * Recursively search for subcommands of a given command, by their full command name. Should absolutely NOT
	 * be called from outside of CommandParser::getCommandProto().
	 * @param pProto The base command which will have it's subcommands analyzed.
	 * @param cmdName The full target command name that is being searched for.
	 */
	static CommandProto* recursiveGetCommandProto(CommandProto* pProto, string cmdName);
};

#endif