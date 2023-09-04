#include <fstream>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <unordered_map>

#include "SqrParser.h"

namespace std {
	
	// --- Utility --- //
	
	enum TKType : signed char {
		TKEOF = -1, TKUNKNOWN, TKNEWLINE, TKSPACE,
		TKARROW,    TKCOLON,   TKWORD
	};
	
	constexpr TKType getCharTKType(char&) noexcept;
	constexpr bool isBoundary(char&, char&&) noexcept;
	
	
	
	// --- Parser --- //
	
	bool SqrParser::debug = false;
	
	int        SqrParser::indAmt = 0;
	Sequencer* SqrParser::sqr    = nullptr;
	tList      SqrParser::tokens   {};
	tlIter     SqrParser::token,
	           SqrParser::end;
	
	Sequencer SqrParser::create(string&& seqFileName) {
		// Lexical analysis
		lex(move(seqFileName));
		
		// Parse
		Sequencer s = parse<Sequencer>();
		
		// Semantic analysis?
		analyze();
		
		// Cleanup and return
		sqr = nullptr;
		s.choice.clear();
		s.establish();
		return s;
	}
	
	
	
	/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *\
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	 $ ###                      ############################################ $
	 $ ###   LEXICAL ANALYSIS   ############################################ $
	 $ ###                      ############################################ $
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	\* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
	
	
	
	void SqrParser::lex(string&& seqFileName) {
		// Open the source file
		ifstream seqFile(move(seqFileName));
		if(seqFile.fail())
			cout << "Error opening \"" << seqFileName << "\"\n",
			exit(1);
		seqFile >> noskipws;
		
		char curr;
		string data = "";
		tokens.clear();
		
		// Iterate over all chars in file to search for tokens
		for(int l=1, c=0; seqFile >> curr;) {
			TKType cType = getCharTKType(curr);
			if(cType != TKSPACE && cType != TKNEWLINE) data += curr;
			c++;
			
			// Merge chars into a token
			if(isBoundary(curr, seqFile.peek()) && data.size()) {
				tokens.push_back(Token::collect(data,l,move(c-data.size())));
				if(SqrParser::debug) tokens.back().print();
				data.clear();
				lexCheck();
			}
			
			// Update location trackers at newline
			if(cType == TKNEWLINE) l++, c=0;
		}
		
		// Establish iterators needed for parsing
		token = tokens.begin();
		end   = tokens.end();
	}
	
	void SqrParser::lexCheck() {
		tlIter c = prev(tokens.end()), p = prev(c);
		if(c->ID == UNKNOWN)
			printError(*c, LEXBADTOKEN);
		else if(
			(p->ID == LARROW || p->ID == MARROW || p->ID == RARROW) &&
			(c->ID == LARROW || c->ID == MARROW || c->ID == RARROW)
		)	printError(*c, LEXDUPARROW);
	}
	
	
	
	/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *\
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	 $ ###             ##################################################### $
	 $ ###   PARSING   ##################################################### $
	 $ ###             ##################################################### $
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	\* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
	
	
	
	template<typename R>
	R SqrParser::parse(string&& type) {
		R result;
		static unordered_map<string,int> counts;
		counts[type]++;
		indAmt++;
		
		if(token == end) {
			result.reset();
			if(debug)
				cout << indent()
					<< "End of token stream (" << type << ", "
					<< result.ID << ", "
					<< result.lineIdx << ", " << result.charIdx
					<< "), sequence has " << sqr->seqs[sqr->choice].subs.size() << " subsequences\n";
		}
		else {
			if(debug)
				cout << indent()
					<< type << ' ' << counts[type] << " start: "
					<< token->lineIdx << ", "
					<< token->charIdx << "...\n";
			
			tlIter checkpoint = token;
			
			/**/ if(typeid(result) == typeid(Sequencer  ))
				(Sequencer   &) result = parseSequencer   ();
			else if(typeid(result) == typeid(Sequence   ))
				(Sequence    &) result = parseSequence    ();
			else if(typeid(result) == typeid(Subsequence))
				(Subsequence &) result = parseSubsequence ();
			else if(typeid(result) == typeid(DirPermit  ))
				(DirPermit   &) result = parseDirPermit   ();
			
			// Some parse subfunctions return the same type, and so use a
			// parse() parameter to specify which subfunction to branch to.
			// Storing and accessing them via a map seems like a decently
			// user-extensible way to handle this for a more generic parser.
			// But is it safe?
			
			else if(typeid(result) == typeid(Message    )) {
				static unordered_map<string,Message(*)()> msgFuncs {
					{ "Message"    , parseMessage    },
					{ "SimpleMsg"  , parseSimpleMsg  },
					{ "ComplexMsg" , parseComplexMsg }
				};
				
				(Message&) result = msgFuncs.at(type)();
			}
			
			else if(typeid(result) == typeid(Token      )) {
				static unordered_map<string,Token(*)()> tokFuncs {
					{ "Arrow" , parseArrow }
				};
				
				(Token&) result = tokFuncs.at(type)();
			}
			
			else
				printError(*token, PARSEBADBRANCH, type);
			
			
			
			if(!result.lineIdx) result.reset();
			
			// Check for errors
			if(debug) {
				cout << indent()
					<< "\e[48;2;"
					<< (result.ID ? "31;63;31" : "127;71;15")
					<< ";38;2;"
					<< (result.ID ? "63;159;63" : "255;135;15")
					<< 'm'
					<< type << ' ' << counts[type] << " -> "
					<< (result.ID ? "finish" : "abort")
					<< "\e[0m" << endl;
				if(result.ID)
					if(type == "Message") {
						const Message& m = (const Message&) result;
						if(m.perYield) cout << indent() << "PERYIELD\n";
						if(m.finYield)
							cout << indent() << "FINYIELD for " << (
								m.finYieldTarget.empty() ?
								"both" : m.finYieldTarget
							) << endl;
						if(!m.perYield && !m.finYield)
							cout << indent() << "NO YIELD\n";
						cout << indent();
						if(m.attempts > 1)
							cout
								<< "MESSAGE " << counts[type]
								<< " WILL TRY " << m.attempts
								<< " TIMES, ONCE EVERY " << m.tryRate
								<< "ms, THEN ABORT\n";
						else
							cout
								<< "MESSAGE " << counts[type]
								<< " WILL WAIT FOR DATA\n";
					}
			}
			
			if(!result.ID)
				counts[type]--,
				token = checkpoint;
		}
		
		indAmt--;
		return result;
	}
	
	Sequencer SqrParser::parseSequencer() {
		Sequencer s;
		sqr = &s;
		
		s.lineIdx = token->lineIdx;
		s.charIdx = token->charIdx;
		
		// SEQUENCE+
		while(parse<Sequence>("Sequence").ID);

		// A valid sequencer only exits the above loop when we reach the end of
		// the token stream.
		if(token != end)
			printError(*token, PARSEFAIL);
		
		// There is no need to delete a malformed Sequence, as the parser will
		// throw an error for this from deeper inside the call stack. However,
		// there may be no Sequences in the file at all! THAT'S when we throw
		// an error in here.
		if(s.seqs.empty())
			printError(s, PARSENOSEQUENCES);
		
		return s;
	}
	
	Sequence SqrParser::parseSequence() {
		Token
			kw = collectLeaf({KWSEQUENCE}), // KWSEQUENCE
			nm = collectLeaf({IDEN      }), // IDEN
			cl = collectLeaf({COLON     }); // COLON
		
		Sequence s;
		
		if(kw.ID && nm.ID && cl.ID) {
			Sequence& sRef = sqr->seqs[sqr->choice = nm.data];
			sRef.lineIdx = kw.lineIdx;
			sRef.charIdx = kw.charIdx;
			
			// SUBSEQUENCE+
			while (
				sRef.subs.emplace_back(),
				parse<Subsequence>("Subsequence").ID
			);
			
			// Pop the trailing subsequence the was emplaced before the parse
			// attempt that killed the loop. (Is there a way to do this so we
			// don't need to create it in the first place? Valid subsequences
			// need to modify an existing instance during parsing...)
			sRef.subs.pop_back();
			
			// If there are no subsequences at this point, the sequence is
			// invalid. Delete it and
			if(sRef.subs.empty())
				s.reset(); 
			else
				s = sRef;
		}
		
		return s;
	}
	
	Subsequence& SqrParser::parseSubsequence() {
		vector<Subsequence>& subs = sqr->seqs[sqr->choice].subs;
		Subsequence& ss = subs.back();
		bool noExplicitPerms;
		
		// DIRPERMIT*
		while(parse<DirPermit>("DirPermit").ID);
		
		if(noExplicitPerms = ss.perms.empty()) {
			if(subs.size() <= 1)
				printError(*token, PARSENOPERMITS);
			else {
				Subsequence& prevss = *prev(subs.end(), 2);
				ss.perms   = prevss.perms,
				ss.parties = prevss.parties;
			}
		}
		
		// MESSAGE+
		while(parse<Message>("Message").ID);
		
		// Reset if no messages. parseSequence() will pop this subsequence off
		// the vector so we don't return a dangling reference here.
		if(ss.messages.empty())
			ss.reset();
		else if(noExplicitPerms)
			ss.lineIdx = ss.messages[0].lineIdx,
			ss.charIdx = ss.messages[0].charIdx;
		
		return ss;
	}
	
	DirPermit SqrParser::parseDirPermit() {
		DirPermit dp;
		if(next(token,3)->ID == COLON) return dp;
		
		Token
			l = collectLeaf ({IDEN }),  d = parse<Token>("Arrow"),
			r = collectLeaf ({IDEN });
		
		if(l.ID && r.ID && d.ID) {
			dp.lineIdx = l.lineIdx;  dp.charIdx = l.charIdx;
			dp.lp      = l.data;     dp.dir     = d.ID;
			dp.rp      = r.data;
			
			Subsequence& sub = sqr->seqs[sqr->choice].subs.back();
			if(sub.perms.empty())
				sub.lineIdx = dp.lineIdx,
				sub.charIdx = dp.charIdx;
			sub.addPermit(dp);
		}
		
		return dp;
	}
	
	Message SqrParser::parseMessage() {
		Message m;
		m.ID = UNKNOWN;
		
		// Message composition
		int stepper = 2;
		string types[stepper] = { "SimpleMsg", "ComplexMsg" };
		while(stepper-- && !m.ID)
			m = parse<Message>(move(types[stepper]));

		if(m.ID) {
			Token
				y = collectLeaf({KWYIELD}),
				r = collectLeaf({KWTRY  }),
				n = collectLeaf({NUM    }),
				p = collectLeaf({NUM    });
			
			if(r.ID && n.ID && p.ID) {
				m.perYield = y.ID != UNKNOWN;
				m.attempts = atoi(n.data.c_str());
				m.tryRate  = atoi(p.data.c_str());
				if(m.attempts < 1)
					printError(n, PARSEBADTRYCOUNT);
				
			}
			else if(y.ID) {
				if(token->ID == KWYIELD || next(token)->ID == KWYIELD)
					printError(y, PARSEDUPRETRY);
				m.finYield = true;
			}
			
			if(next(token)->ID == KWYIELD) {
				Token t = collectLeaf({IDEN});
				
				if(t.ID && next(token)->ID == KWTRY)
					printError(t, PARSEBADPERYIELD);
				
				if(!t.ID || t.data != m.src && t.data != m.dst)
					printError(*token, PARSEBADFINYIELD);
				
				m.finYield = true;
				m.finYieldTarget = t.data;
				
				// Don't need the yield token, just need to collect it
				collectLeaf({KWYIELD});
			}
			else if(collectLeaf({KWYIELD}).ID)
				m.finYield = true;
			
			sqr->seqs[sqr->choice].subs.back().addMessage(m);
		}
		
		return m;
	}
	
	Message SqrParser::parseSimpleMsg() {
		Message sm;
		
		Token
			d = parse<Token>("Arrow"),  n = collectLeaf ({IDEN });
		
		if(d.ID && n.ID) {
			Subsequence& ss = sqr->seqs[sqr->choice].subs.back();
			
			DirPermit& f = const_cast<DirPermit&>(*ss.perms.begin());
			
			if(ss.parties.size() > 2)
				printError(d, PARSETOOMANYSIMPLE);
			
			if(ss.perms.size() == 2) {
				const DirPermit& l = *next(ss.perms.begin());
				if(f.lp != l.lp || f.rp != l.rp)
					printError(d, PARSEBADSIMPLEPERMS);
			}
			
			sm.lineIdx = d.lineIdx;
			sm.charIdx = d.charIdx;
			sm.set(f.lp, d.ID, f.rp, n.data);
		}
		
		return sm;
	}
	
	Message SqrParser::parseComplexMsg() {
		Message cm;
		
		Token
			l = collectLeaf ({IDEN}),  d = parse<Token>("Arrow"),
			r = collectLeaf ({IDEN}),  c = collectLeaf ({COLON}),
			n = collectLeaf ({IDEN});
		
		if(l.ID && r.ID && c.ID && n.ID && d.ID)
			cm.lineIdx = l.lineIdx,
			cm.charIdx = l.charIdx,
			cm.set(l.data, d.ID, r.data, n.data);
		
		return cm;
	}
	
	Token SqrParser::parseArrow() {
		return collectLeaf({LARROW, MARROW, RARROW});
	}
	
	Token SqrParser::collectLeaf(unordered_set<TKID>&& okTypes) {
		static int leafNum = 0;
		indAmt++;
		leafNum++;
		
		Token t;
		
		if(token == end) {
			if(debug) cout << indent() << "End of token stream\n";
			leafNum--;
		}
		else {
			// Set t as the next token if no okTypes are given OR if the next
			// token's ID is one of the given okTypes.
			bool match;
			if(match = okTypes.empty() || okTypes.count(token->ID))
				t = *token++;
			
			if(debug) {
				Token& print = *(match ? prev(token) : token);
				cout << indent() << "\e[48;2;"
					<< (match ? "31;63;31" : "127;71;15")
					<< ";38;2;"
					<< (match ? "63;159;63" : "255;135;15")
					<< 'm'
					<< "Leaf "        << leafNum << ": \""
					<< print.data    << "\"; "
					<< print.lineIdx << ", "
					<< print.charIdx << " (expected { ";
				
				for(const TKID& id : okTypes)
					cout << id << ' ';
				
				cout << "}, got "
					<< print.ID << ") -> " << (match ? "finish" : "abort")
					<< "\e[0m\n";
			}
			
			if(!match) leafNum--;
		}
		
		indAmt--;
		return t;
	}
	
	
	
	/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *\
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	 $ ###                       ########################################### $
	 $ ###   SEMANTIC ANALYSIS   ########################################### $
	 $ ###                       ########################################### $
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	\* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
	
	// Semantic analysis:
	// - Duplicate direction permits for a given subsequence are not allowed.
	// - Simple messages may not be used in a subsequence where more than 2
	//   parties are permitted to communicate.
	// - Simple messages may only be used in a subsequence where all direction
	//   permits list their parties in the same order from left to right.
	// - Complex messages may only involve parties mentioned in their
	//   containing subsequence
	// - Messages must adhere to the most recent set of direction permits.
	
	void SqrParser::analyze() {
		// I think everything was already checked during parsing...
	}
	
	
	
	/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *\
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	 $ ###                     ############################################# $
	 $ ###   PARSE UTILITIES   ############################################# $
	 $ ###                     ############################################# $
	 $ @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ $
	\* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
	
	// --- Parse process --- //
	
	string SqrParser::indent() noexcept {
		string ind = "";
		for(int i = 0; i < SqrParser::indAmt; i++)
			ind += "⁞ ";
		return ind;
	}
	
	constexpr TKType getCharTKType(char& c) noexcept {
		     if(c == char_traits<char>::eof()   ) return TKEOF     ;
		else if(c == '\n'                       ) return TKNEWLINE ;
		else if(isspace(c)                      ) return TKSPACE   ;
		else if(c == '<' || c == '-' || c == '>') return TKARROW   ;
		else if(c == ':'                        ) return TKCOLON   ;
		else if(c == '_' || isalnum(c)          ) return TKWORD    ;
		else return TKUNKNOWN;
	}
	
	constexpr bool isBoundary(char& F, char&& S) noexcept {
		TKType fType = getCharTKType(F);
		return
			!S               || fType != getCharTKType(S) &&
			fType != TKSPACE && fType != TKNEWLINE        ||
			F == '>'         || S == '<'                  ||
			F == '-'         && S == '-'                  ||
			S == char_traits<char>::eof()
		;
	}
}