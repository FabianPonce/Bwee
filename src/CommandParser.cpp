#include "StdAfx.h"

CommandCreateMap m_createMap;
size_t m_commandParserCount = 0;
Mutex m_commandParserMutex;

#define REGISTER_COMMAND(cmdName, createFunction) \
	{ \
	CommandProto* cp = new CommandProto(); \
	cp->createFunc = createFunction; \
	cp->commandName = cmdName; \
	registerCommand(cp); \
	}

void CommandParser::registerCommands()
{
	// Subcommands must be registered AFTER the command they are based upon.
	REGISTER_COMMAND("test", &TestCommand::Create);
	REGISTER_COMMAND("help", &HelpCommand::Create);
	REGISTER_COMMAND("commands", &CommandsCommand::Create);
	REGISTER_COMMAND("realms", &RealmsCommand::Create);
	REGISTER_COMMAND("online", &OnlineCommand::Create);
	REGISTER_COMMAND("topten", &TopTenCommand::Create);
	REGISTER_COMMAND("playerinfo", &PlayerInfoCommand::Create);
	REGISTER_COMMAND("upcomingplayers", &UpcomingPlayersCommand::Create);
}

void CommandParser::registerCommand(CommandProto* cp)
{
	WordStringReader uReader(cp->commandName);
	string uPreviousCommand = uReader.getNextWord();
	if(uReader.hasNextWord())
	{
		string uCurrentCommand;
		bool b = true;
		while(uReader.hasNextWord())
		{
			if( !b )
				uPreviousCommand = uCurrentCommand;
			b = false;
			uCurrentCommand = uReader.getNextWord();
		}

		CommandProto* uCmdProto = getCommandProto(uPreviousCommand);
		if(!uCmdProto)
		{
			Log.Error("CommandParser", "Unable to register command %s.", cp->commandName.c_str());
			return;
		}

		uCmdProto->m_subCommandMap.insert( make_pair(uCurrentCommand, cp) );
	}
	else
		m_createMap.insert( make_pair(cp->commandName, cp) );
}

Command::Command(IRCSession* pSession, string target, string sender, string text) : m_reader(text)
{
	m_session = pSession;
	m_target = target;
	m_sender = sender;
}

CommandParser::CommandParser(IRCSession* pSession) : m_session(pSession)
{
	Guard g(m_commandParserMutex);
	if( m_commandParserCount == 0 )
		registerCommands();

	m_commandParserCount++;
}

CommandParser::~CommandParser()
{
	Guard g(m_commandParserMutex);
	if( --m_commandParserCount <= 0 )
	{
		CommandCreateMap::iterator itr = m_createMap.begin();
		for(; itr != m_createMap.end(); ++itr)
		{
			delete itr->second;
		}
		m_createMap.clear();
	}
};

bool CommandParser::recursiveExecuteCommand(CommandProto* pBaseCmd, string target, string sender, string text)
{
	if( pBaseCmd->m_subCommandMap.empty() )
		return false;

	WordStringReader uReader(text);
	if( !uReader.hasNextWord() )
		return false;

	string uSearcher = uReader.getNextWord();

	map<string, CommandProto*>::iterator itr = pBaseCmd->m_subCommandMap.begin();
	for(; itr != pBaseCmd->m_subCommandMap.end(); ++itr)
	{
		string uSubCommand = itr->first;
		if( strnicmp( uSearcher.c_str(), uSubCommand.c_str(), uSearcher.length()) == 0 )
		{
			// We have a matching command!
			if( recursiveExecuteCommand( itr->second, target, sender, uReader.getRemainder() ) )
				return true;

			// Otherwise, we didn't return properly back to thsi stage. Run as a normal command?
			CommandCreate cr = itr->second->createFunc;
			Command * c = (*cr)(m_session, target, sender, uReader.getRemainder());
			if( c->isSyntaxOk() )
				c->run();
			else
				sendHelpTextForCommand(c, target);

			return true;
		}
	}

	return false;
}

void CommandParser::executeCommand(string target, string sender, string text)
{
	// get the first word of the text, exclude the first letter.
	WordStringReader uReader(text.substr(1));
	string commandName = uReader.getNextWord();
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		// is this the right command?
		if( strnicmp(itr->second->commandName.c_str(), commandName.c_str(), commandName.length()) == 0 )
		{
			CommandProto* uCmdProto = itr->second;
			// are there subcommands after this command?
			if( recursiveExecuteCommand(uCmdProto, target, sender, uReader.getRemainder() ) )	
				return;

			// run our command.
			CommandCreate cr = itr->second->createFunc;
			Command * c = (*cr)(m_session, target, sender, uReader.getRemainder());
			if( c->isSyntaxOk() )
				c->run();
			else
				sendHelpTextForCommand(c, target);

			return;
		}
	}
}

void CommandParser::sendHelpTextForCommand(string commandName, string target)
{
	if( CommandProto* cp = getCommandProto(commandName) )
	{
		CommandCreate cr = cp->createFunc;
		Command * uCommand = (Command*) (*cr)(m_session, target, "", "");
		sendHelpTextForCommand(uCommand, target);
		delete uCommand;
		return;
	}

	m_session->SendChatMessage(PRIVMSG, target.c_str(), "There is no such command.");
}

void CommandParser::sendHelpTextForCommand(Command* c, string target)
{
	string uHelpText = c->getHelpText();

	if( uHelpText.length() == 0 )
	{
		m_session->SendChatMessage(PRIVMSG, target.c_str(), DEFAULT_TEXT_HELP_NOT_AVAILABLE);
		return;
	}

	size_t uLastLinePosition = -1; // Yes, this is right. We'll integer overflow into 0 in a moment for the first call.
	size_t uNewLinePosition = uHelpText.find('\n');
	if( uNewLinePosition == string::npos )
	{
		m_session->SendChatMessage(PRIVMSG, target.c_str(), uHelpText.c_str());
		return;
	}
	
	while( uNewLinePosition != string::npos )
	{
		size_t uCount = uNewLinePosition - uLastLinePosition;
		string uHelpTextPart = uHelpText.substr(uLastLinePosition+1, uNewLinePosition - uLastLinePosition - 1);
		m_session->SendChatMessage(PRIVMSG, target.c_str(), uHelpTextPart.c_str());
		uLastLinePosition = uNewLinePosition;
		uNewLinePosition = uHelpText.find('\n', uLastLinePosition+1);
	}

	string uHelpTextPart = uHelpText.substr( uLastLinePosition + 1 );
	m_session->SendChatMessage(PRIVMSG, target.c_str(), uHelpTextPart.c_str());
}

string CommandParser::buildCommandList()
{
	stringstream ss;
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		ss << itr->first;
		if( !itr->second->m_subCommandMap.empty() )
			ss << "*";

		ss << ", ";
	}
	string uRet = ss.str();
	// Remove last ", "
	uRet = uRet.substr(0, uRet.length()-2);
	return uRet;
}

string CommandParser::buildCommandList(string commandName)
{
	CommandProto* cp = getCommandProto(commandName);
	if(!cp)
		return "No such command.";

	if( cp->m_subCommandMap.empty() )
		return "No subcommands.";

	stringstream ss;
	ss << commandName << " commands: ";
	map<string, CommandProto*>::iterator itr = cp->m_subCommandMap.begin();
	for(; itr != cp->m_subCommandMap.end(); ++itr)
	{
		ss << itr->first;
		if( !itr->second->m_subCommandMap.empty() )
			ss << "..";

		ss << ", ";
	}
	string uRet = ss.str();
	uRet = uRet.substr(0, uRet.length()-2);
	return uRet;
}

CommandProto* CommandParser::recursiveGetCommandProto(CommandProto* pProto, string cmdName)
{
	if( pProto->m_subCommandMap.empty() )
		return NULL;

	WordStringReader uReader(cmdName);
	if( !uReader.hasNextWord() )
		return NULL;

	string uSearcher = uReader.getNextWord();

	map<string, CommandProto*>::iterator itr = pProto->m_subCommandMap.begin();
	for(; itr != pProto->m_subCommandMap.end(); ++itr)
	{
		string uSubCommand = itr->first;
		if( strnicmp( uSearcher.c_str(), uSubCommand.c_str(), uSearcher.length()) == 0 )
		{
			// We have a matching command!
			if( CommandProto* cp = recursiveGetCommandProto( itr->second, uReader.getRemainder() ) )
				return cp;

			return itr->second;
		}
	}

	return NULL;
}

CommandProto* CommandParser::getCommandProto(string cmdName)
{
	// get the first word of the text, exclude the first letter.
	WordStringReader uReader(cmdName);
	string commandName = uReader.getNextWord();
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		if( strnicmp(itr->second->commandName.c_str(), commandName.c_str(), commandName.length()) == 0 )
		{
			CommandProto* uCmdProto = itr->second;
			if( CommandProto* cp = recursiveGetCommandProto(uCmdProto, uReader.getRemainder() ) )	
				return cp;

			return itr->second;
		}
	}
	return NULL;
}