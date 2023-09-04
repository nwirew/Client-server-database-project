#pragma once

#include <list>
#include <unordered_set>

#include "Sequencer.h"

namespace std {
	using tList = list<Token>;
	using tlIter = tList::iterator;
	class SqrParser {
	private:
		static int indAmt;
		static Sequencer* sqr;
		static tList tokens;
		static tlIter token, end;
		
		// Lexical analysis
		static void lex(string&&);
		static void lexCheck();
		
		// Parse
		template<typename R>
		static R parse(string&& = "Sequencer");
		
		static Sequencer    parseSequencer   ();
		static Sequence     parseSequence    ();
		static Subsequence& parseSubsequence ();
		static DirPermit    parseDirPermit   ();
		static Message      parseMessage     ();
		static Message      parseSimpleMsg   ();
		static Message      parseComplexMsg  ();
		static Token        parseArrow       ();
		static Token        collectLeaf      (unordered_set<TKID>&& = {});
		
		// Semantic analysis?
		static void analyze();
		
		// Utilities
		static string indent() noexcept;
		
	public:
		static bool debug;
		static Sequencer create(string&&);
	};
}