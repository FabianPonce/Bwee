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
	REGISTER_COMMAND("online", &OnlineCommand::Create);
	REGISTER_COMMAND("topten", &TopTenCommand::Create);
	REGISTER_COMMAND("playerinfo", &PlayerInfoCommand::Create);
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
	getNextWord(); // throw out the first result as the command name.
}

bool Command::hasNextWord()
{
	if( m_readPos >= m_text.length() - 1 )
		return false;

	return true;
}

string Command::getNextWord()
{
	size_t endPosition = m_text.find( ' ', m_readPos+2 );
	// it's ok for endPosition == string::npos! we will return the remainder
	string ret = m_text.substr(m_readPos+1, endPosition );
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

			delete uCommand;
		}
	}
}