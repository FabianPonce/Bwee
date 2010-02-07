#include "StdAfx.h"

CommandCreateMap m_createMap;

#define REGISTER_COMMAND(cmdName, createFunction) \
	{ \
	CommandProto* cp = new CommandProto(); \
	cp->createFunc = createFunction; \
	cp->commandName = cmdName; \
	registerCommand(cp); \
	}

void CommandParser::registerCommands()
{
	REGISTER_COMMAND("test", &TestCommand::Create);
	REGISTER_COMMAND("help", &HelpCommand::Create);
	REGISTER_COMMAND("commands", &CommandsCommand::Create);
	REGISTER_COMMAND("realms", &RealmsCommand::Create);
	REGISTER_COMMAND("online", &OnlineCommand::Create);
	REGISTER_COMMAND("topten", &TopTenCommand::Create);
	REGISTER_COMMAND("playerinfo", &PlayerInfoCommand::Create)
}

void CommandParser::registerCommand(CommandProto* cp)
{
	m_createMap.insert( make_pair(cp->commandName, cp) );
}

Command::Command(IRCSession* pSession, string target, string sender, string text)
{
	m_session = pSession;
	m_target = target;
	m_sender = sender;
	m_text = text;
	m_readPos = 0;
	if( hasNextWord() )
		getNextWord(); // throw out the first result as the command name.
}

bool Command::hasNextWord()
{
	if( m_text.length() == 0 )
		return false;

	if( m_readPos >= m_text.length() - 1 )
		return false;

	return true;
}

string Command::getNextWord()
{
	size_t endPosition = m_text.find( ' ', m_readPos+1 );

	size_t uCount = endPosition - m_readPos - 1;
	if( endPosition == string::npos )
	{
		size_t uReadPos = m_readPos + 1;
		m_readPos = m_text.length();
		return m_text.substr(uReadPos);
	}

	string ret = m_text.substr(m_readPos+1, uCount );
	m_readPos = endPosition;
	return ret;
}

string Command::getRemainder()
{
	return m_text.substr(m_readPos+2);
}

CommandParser::CommandParser(IRCSession* pSession) : m_session(pSession)
{

}

CommandParser::~CommandParser()
{

};

void CommandParser::executeCommand(string target, string sender, string text)
{
	// get the first word of the text, exclude the first letter.
	size_t spaceLoc = text.find(' '); // again, ok for string::npos
	string commandName = text.substr(1, (spaceLoc == string::npos) ? spaceLoc : spaceLoc-1);
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		if( strnicmp(itr->second->commandName.c_str(), commandName.c_str(), commandName.length()) == 0 )
		{
			CommandCreate cr = itr->second->createFunc;
			Command * uCommand = (Command*) (*cr)(m_session, target, sender, text);

			if( uCommand->isSyntaxOk() )
				uCommand->run();
			else
				sendHelpTextForCommand(uCommand, target);

			delete uCommand;
		}
	}
}

void CommandParser::sendHelpTextForCommand(string commandName, string target)
{
	// Locate the command we're searching for.
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		if( strnicmp(commandName.c_str(), itr->first.c_str(), commandName.length()) == 0 )
		{
			CommandCreate cr = itr->second->createFunc;
			Command * uCommand = (Command*) (*cr)(m_session, target, "", "");
			sendHelpTextForCommand(uCommand, target);
			delete uCommand;
			return;
		}
	}

	m_session->SendChatMessage(PRIVMSG, target.c_str(), "There is no such command.");
}

void CommandParser::sendHelpTextForCommand(Command* c, string target)
{
	string uHelpText = c->getHelpText();

	if( uHelpText.length() == 0 )
	{
		m_session->SendChatMessage(PRIVMSG, target.c_str(), "Help for this command is not available.");
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
	std::stringstream ss;
	CommandCreateMap::iterator itr = m_createMap.begin();
	for(; itr != m_createMap.end(); ++itr)
	{
		ss << itr->first << ", ";
	}
	string uRet = ss.str();
	// Remove last ", "
	uRet = uRet.substr(0, uRet.length()-2);
	return uRet;
}