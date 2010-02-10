#include "StdAfx.h"

WordStringReader::WordStringReader(std::string text)
{
	m_readPos = 0;
	m_text = text;
}
bool WordStringReader::isEmpty()
{
	return (m_text.length() == 0);
}

bool WordStringReader::isFirstRead()
{
	return (m_readPos == 0);
}

bool WordStringReader::isSingleWord()
{
	return (m_text.find(' ') == string::npos);
}

bool WordStringReader::hasNextWord()
{
	if( isEmpty() )
		return false;

	if( isSingleWord() )
		return (m_readPos == 0);
	
	size_t uNextSpacePosition = m_text.find(' ', m_readPos+1);
	if( uNextSpacePosition == string::npos && m_readPos != m_text.length() )
		return true;
	
	if( uNextSpacePosition != string::npos )
		return true;

	return false;
}

string WordStringReader::getNextWord()
{
	if( isSingleWord() )
	{
		m_readPos = m_text.length();
		return m_text;
	}

	size_t uNextSpacePosition = m_text.find(' ', m_readPos+1);
	if( uNextSpacePosition == string::npos && m_readPos != m_text.length() )
	{
		size_t count = m_text.length() - m_readPos - 1;
		size_t start = m_readPos;
		m_readPos = m_text.length();
		return m_text.substr(start+1, count);
	}

	if( uNextSpacePosition != string::npos )
	{
		size_t start = isFirstRead() ? m_readPos : m_readPos + 1;
		size_t count = uNextSpacePosition - start;
		m_readPos = uNextSpacePosition;
		return m_text.substr(start, count);
	}

	return "";
}

string WordStringReader::getRemainder()
{
	if( m_readPos >= m_text.length() )
		return "";

	size_t startPos = isFirstRead() ? m_readPos : m_readPos + 1;
	return m_text.substr(startPos);
}