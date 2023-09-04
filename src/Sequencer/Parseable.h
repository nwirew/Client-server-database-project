#pragma once

#include <string>

namespace std {
	enum TKID : unsigned char {
	//	Error token
		UNKNOWN = 0,  //............................................ 00 - 00
	//	Control tokens
		ALPHANUM, NUM, OR, ONE, ANY, SOME,  //...................... 01 - 06
	//	Symbol tokens (9 - 13 are whitespace)
		KWSEQUENCE, KWTRY, LARROW = 14, MARROW, RARROW,  //......... 07 - 16
	//	Level 1 tokens
		COLON, IDEN,  //............................................ 17 - 18
	//	Level 2 tokens
		SEQUENCENAME, ARROW, SIMPLEMSG, COMPLEXMSG,  //............. 19 - 22
	//	Level 3 tokens
		DIRPERMIT, MESSAGE, KWYIELD,  //............................ 23 - 25
	//	Level 4 tokens
		SUBSEQUENCE,  //............................................ 26 - 26
	//	Level 5 tokens
		SEQUENCE,  //............................................... 27 - 27
	//	Top level token
		SEQUENCELIST  //............................................ 28 - 28
	};
	
	enum errType {
		LEXBADTOKEN, LEXDUPARROW,
		
		PARSEBADBRANCH, PARSEFAIL, PARSENOSEQUENCES, PARSENOPERMITS,
		PARSETOOMANYSIMPLE, PARSEBADTRYCOUNT, PARSEBADPERYIELD,
		PARSEBADFINYIELD, PARSEDUPRETRY, PARSEBADSIMPLEPERMS,
		
		MESSAGEBADDIR,
		
		SUBSEQBADSRC, SUBSEQBADDST, SUBSEQBADMSG
	};
	
	class Parseable {
	private:
		virtual void subReset() = 0;
	public:
		static int indAmt;
		TKID ID;
		int lineIdx;
		int charIdx;
		Parseable();
		Parseable(TKID, int&,  int& );
		Parseable(TKID, int&&, int&&);
		void reset();
	};
	
	
	
	/* --------------------------------------------------------------------- *\
	 * --- PARSE UTILITIES ------------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	void printError(const Parseable&, errType&&, string = "");
}