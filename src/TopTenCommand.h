#ifndef _TOPTEN_COMMAND_H
#define _TOPTEN_COMMAND_H
#include "StdAfx.h"

class TopTenCommand : public Command
{
private:
	Realm * m_realm;
	string m_field;

public:
	TopTenCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{
	}

	virtual const char* getHelpText() { return "Format: !topten $RealmName $Property\nFor a list of realms, type !realms.\nProperties include: level, gold, hkstoday, kills, honor, playedtime."; }

	bool isSyntaxOk()
	{
		GET_REALM_FROM_PARAM_OR_FAIL(m_realm);

		if( !hasNextWord() )
			return false;

		m_field = getNextWord();

		if( !checkField() )
			return false;

		return true;
	}

	bool checkField()
	{
		if(
			m_field == "level" ||
			m_field == "gold" ||
			m_field == "hkstoday" ||
			m_field == "kills" ||
			m_field == "honor" ||
			m_field == "playedtime")
			return true;

		return false;
	}

	const char* getDatabaseField()
	{
		if(m_field == "level")
			return "level";
		else if(m_field == "gold")
			return "money";
		else if(m_field == "hkstoday")
			return "killsToday";
		else if(m_field == "kills")
			return "totalKills";
		else if(m_field == "honor")
			return "totalHonorPoints";
		else if(m_field == "playedtime")
			return "totaltime";
	}

	const char* getDatabaseOrder()
	{
		return "DESC";
	}

	const char* getValueModifier()
	{
		if( m_field == "gold" )
			return "/10000";

		return "";
	}

	void run()
	{
		QueryResult* result = m_realm->GetDB()->Query("SELECT name,%s%s FROM characters WHERE guid NOT IN (SELECT guid FROM characters WHERE playerflags & 0x08) ORDER BY %s %s LIMIT 10", getDatabaseField(), getValueModifier(), getDatabaseField(), getDatabaseOrder());
		if(!result)
			return;

		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "+----------------------------------------------+");
		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), "|   Name                         |    Value    |");
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
		return new TopTenCommand(pSession, target, sender, text);
	}
};

#endif