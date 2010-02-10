#ifndef _WORD_STRING_H
#define _WORD_STRING_H

class WordStringReader
{
private:
	std::string m_text;
	size_t m_readPos;

	bool isEmpty();
	bool isSingleWord();
	bool isFirstRead();

public:
	WordStringReader(std::string text);
	
	/*
	* Returns true if there is a word ahead in the m_text string. Works by space-delimiting.
	*/
	bool hasNextWord();

	/*
	* Returns the next word in m_text. This method does not call hasNextWord() before execution. You must ensure
	* that there is a next word, or an exception will be thrown.
	*/
	string getNextWord();

	/*
	 * getReadPosition
	 */
	size_t getReadPosition() { return m_readPos; }
	void setReadPosition(size_t rPos) { m_readPos = rPos; }

	/*
	* Returns the remainder of the m_text that hasn't been parsed by getNextWord(). This method does not call hasNextWord()
	* before execution. You must ensure that there is a next word, or an exception will be thrown.
	*/
	string getRemainder();
};
#endif