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
	/*
	 * Initialize a new WordStringReader to iterate over text.
	 * @param text The text that will be read by this object. Must not be an empty string.
	 */
	WordStringReader(std::string text);
	
	/*
	* Returns true if there is a word ahead in the m_text string.
	*/
	bool hasNextWord();

	/*
	* Returns the next word in m_text. This method does not call hasNextWord() before execution. You must ensure
	* that there is a next word, or an exception will be thrown.
	*/
	string getNextWord();

	/*
	 * Returns the current read position we're working from.
	 */
	size_t getReadPosition() { return m_readPos; }
	/*
	 * Sets the read position in the string.
	 * @param rPos The new read position.
	 */
	void setReadPosition(size_t rPos) { m_readPos = rPos; }

	/*
	* Returns the remainder of the m_text that hasn't been parsed by getNextWord(). This method does not call hasNextWord()
	* before execution. You must ensure that there is a next word, or an exception will be thrown.
	*/
	string getRemainder();
};
#endif