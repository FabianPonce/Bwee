#ifndef _PLAYERINFO_COMMAND_H
#define _PLAYERINFO_COMMAND_H
#include "StdAfx.h"

class PlayerInfoCommand : public Command
{
private: 
	string m_playerName;
	Realm* m_realm;

public:
	PlayerInfoCommand(IRCSession* pSession, string target, string sender, string text) : Command(pSession, target, sender, text)
	{

	}

	bool isSyntaxOk()
	{
		GET_REALM_FROM_PARAM_OR_FAIL(m_realm);

		if( !hasNextWord() )
			return false;

		m_playerName = getNextWord();
		return true;
	}

	void run()
	{
		QueryResult * result = m_realm->GetDB()->Query("SELECT class,race,money,totaltime,online,level,gender FROM characters WHERE name = '%s'", m_realm->GetDB()->EscapeString(m_playerName).c_str());
		if(!result)
			return;

		uint32 iClass = result->Fetch()[0].GetUInt32();
		uint32 iRace = result->Fetch()[1].GetUInt32();
		uint32 iMoney = result->Fetch()[2].GetUInt32();
		uint32 iPlayed = result->Fetch()[3].GetUInt32();
		bool iOnline = result->Fetch()[4].GetBool();
		uint32 iLevel = result->Fetch()[5].GetUInt32();
		uint32 iGender = result->Fetch()[6].GetUInt32();
		const char* sGender = (iGender == 1) ? "She" : "He";

		GetSession()->SendChatMessage(PRIVMSG, m_target.c_str(), 
			"%s is a level %u %s %s with %u gold after playing for %u hours. %s is currently %s.",
			m_playerName.c_str(),
			iLevel,
			RaceToString(iRace),
			ClassToString(iClass),
			iMoney / 100000,
			iPlayed / 3600,
			sGender,
			(iOnline) ? "online" : "offline"
		);
		delete result;
	}

	static Command* Create(IRCSession* pSession, string target, string sender, string text)
	{
		return new PlayerInfoCommand(pSession, target, sender, text);
	}
};

#endif