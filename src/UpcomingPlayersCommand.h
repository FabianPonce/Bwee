#ifndef _UPCOMINGPLAYERS_COMMAND_H
#define _UPCOMINGPLAYERS_COMMAND_H
#include "StdAfx.h"

class UpcomingPlayersCommand : public Command
{
private:
	Realm* m_realm;

public:
	UpcomingPlayersCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	virtual const char* getHelpText() { return "Format: !upcomingplayers $RealmName.\nFor a list of realms, type !realms."; }

	bool isSyntaxOk()
	{
		if( !(m_realm = ReadRealm()) )
			return false;

		return true;
	}

	void run()
	{
		QueryResult result = m_realm->GetDB()->Query("SELECT name,level FROM characters WHERE level < 80 AND guid NOT IN (SELECT guid FROM characters WHERE playerflags & 0x8) ORDER BY level DESC,totaltime ASC,online DESC LIMIT 10;");
		if(!result)
			return;

		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "+----------------------------------------------+");
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "|   Name                         |    Level    |");
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "+----------------------------------------------+");
		do 
		{
			const char* name = result->Fetch()[0].GetString();
			uint32 val = result->Fetch()[1].GetUInt32();
			GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "| %30s | %11u |", name, val);
		} while (result->NextRow());
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "+----------------------------------------------+");
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new UpcomingPlayersCommand(pSession, target, sender, text);
	}
};

#endif