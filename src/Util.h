#ifndef _UTIL_H
#define _UTIL_H

#include "StdAfx.h"

namespace Bwee
{
	class Util
	{
	public:
		static const char* GetNickFromSource(std::string src)
		{
			size_t itr = src.find('!');
			if( itr == std::string::npos )
				return src.c_str();

			return src.substr(0, itr-1).c_str();
		}

		static vector<string> StrSplit(const string &src, const string &sep)
		{
			vector<string> r;
			string s;
			for (string::const_iterator i = src.begin(); i != src.end(); i++) {
				if (sep.find(*i) != string::npos) {
					if (s.length()) r.push_back(s);
					s = "";
				} else {
					s += *i;
				}
			}
			if (s.length()) r.push_back(s);
			return r;
		}

		string StrVectorImplode(std::vector<string>& v, uint32 i)
		{
			std::stringstream ss;
			for( ; i < v.size(); ++i)
				ss << v[i];

			return ss.str();
		}

	};
}

#endif