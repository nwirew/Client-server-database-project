#include <iostream>
#include <string>

#include "Parseable.h"

namespace std {
	/* --------------------------------------------------------------------- *\
	 * --- PARSEABLE MASTER SUPERCLASS ------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	
	
	// --- Static --- //
	
	int Parseable::indAmt = 0;
	
	
	
	// --- Constructors --- //
	
	Parseable::Parseable(/*--------------------*/) :
	  ID(UNKNOWN), lineIdx(0), charIdx(0) {}
	Parseable::Parseable(TKID i, int&  l, int&  c) :
	  ID(   i   ), lineIdx(l), charIdx(c) {}
	Parseable::Parseable(TKID i, int&& l, int&& c) :
	  ID(   i   ), lineIdx(l), charIdx(c) {}
	
	
	
	// --- Private --- //
	
	void Parseable::subReset() {}
	
	
	
	// --- Public --- //
	
	void Parseable::reset() {
		ID = UNKNOWN;
		lineIdx = 0;
		charIdx = 0;
		subReset();
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	 * --- UTILITIES ------------------------------------------------------- *
	\* --------------------------------------------------------------------- */

	
	
	void printError(const Parseable& p, errType&& e, string s) {
		if(!cout.eof()) cout << endl;
		string msg = "Sequencer "
			"line " + to_string(p.lineIdx) + ", "
			"char " + to_string(p.charIdx) + " - ";
		
		// Lex
		     if(e == LEXBADTOKEN)
			msg += "Lexer could not identify token";
		else if(e == LEXDUPARROW)
			msg += "Duplicate arrow token. Maybe you want bidirectional "
			"\"<->\"?";
		
		// Parse
		else if(e == PARSEBADBRANCH)
			msg += "Parse tree at unknown branch: " + s;
		else if(e == PARSEFAIL)
			msg += "Start of unrecognized token sequence";
		else if(e == PARSENOSEQUENCES)
			msg += "No sequences found";
		else if(e == PARSENOPERMITS)
			msg += "Sequence's first subsequence must establish direction "
			"permits";
		else if(e == PARSETOOMANYSIMPLE)
			msg += "Too many participants for simple message format";
		else if(e == PARSEBADTRYCOUNT)
			msg += "Number of tries must be positive";
		else if(e == PARSEBADPERYIELD)
			msg += "YIELD keyword in per-message context applies to both "
			"message participants. Please do not specify a participant.";
		else if(e == PARSEBADFINYIELD)
			msg += "YIELD keyword in message-finish context must be preceded "
			"by a try statement, one of the message participants, or the end "
			"of the message.";
		else if(e == PARSEDUPRETRY)
			msg += "Message includes per-message yield context but does not "
			"include a retry statement";
		else if(e == PARSEBADSIMPLEPERMS)
			msg += "Permissions for simple messages must maintain participant "
			"positions";
		
		// Message
		else if(e == MESSAGEBADDIR)
			msg += "Messages must use left or right arrow to specify direction";
		
		// Subsequence
		else if(e == SUBSEQBADSRC)
			msg += "Message source \"" + s + "\" is not permitted in this "
			"subsequence";
		else if(e == SUBSEQBADDST)
			msg += "Message destination \"" + s + "\" is not permitted in "
			"this subsequence";
		else if(e == SUBSEQBADMSG)
			msg += "This message is not permitted by its subsequence";
		
		// Default
		else
			msg += "Error";
		
		throw runtime_error(msg);
	}
}