#ifndef _SHARED_DEFINES_H
#define _SHARED_DEFINES_H

#include "StdAfx.h"
#define BWEE_CONFIGURATION_FILE "./bwee.conf"

class _QueryResult;
typedef std::tr1::shared_ptr<_QueryResult> QueryResult;

enum Classes
{
	CLASS_WARRIOR = 1,
	CLASS_PALADIN = 2,
	CLASS_HUNTER = 3,
	CLASS_ROGUE = 4,
	CLASS_PRIEST = 5,
	CLASS_DEATHKNIGHT = 6,
	CLASS_SHAMAN = 7,
	CLASS_MAGE = 8,
	CLASS_WARLOCK = 9,
	CLASS_DRUID = 11,
};

BWEE_INLINE const char* ClassToString(uint32 c)
{
	if( c > 11 )
		return "";

	const char* c2s[] = { "", "warrior", "paladin", "hunter", "rogue", "priest", "death knight", "shaman", "mage", "warlock", "", "druid" };
	return c2s[c];
}

enum Races
{
	RACE_HUMAN = 1,
	RACE_ORC = 2,
	RACE_DWARF = 3,
	RACE_NIGHTELF = 4,
	RACE_UNDEAD = 5,
	RACE_TAUREN = 6,
	RACE_GNOME = 7,
	RACE_TROLL = 8,
	RACE_BLOODELF = 10,
	RACE_DRAENEI = 11,
};

BWEE_INLINE const char* RaceToString(uint32 r)
{
	if( r > 11 )
		return "";

	const char* r2s[] = { "", "human", "orc", "dwarf", "night elf", "undead", "tauren", "gnome", "troll", "", "blood elf", "draenei" };
	return r2s[r];
}

#define GET_REALM_OR_FAIL(realm,variable) \
	variable = GetSession()->GetRealm( GetSession()->GetRealmID(realm) ); \
	if( !variable ) \
		return false;

#define GET_REALM_FROM_PARAM_OR_FAIL(var) \
	if( !hasNextWord() ) \
	return false; \
	\
	GET_REALM_OR_FAIL(getNextWord(),var)

#endif