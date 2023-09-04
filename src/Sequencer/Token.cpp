#include <iomanip>
#include <iostream>

#include "Token.h"

namespace std {
	
	// --- Constructors --- //
	
	Token::Token(/*-------------------------------------------*/)
	:	data(""), Parseable(       ) {}
	
	Token::Token(TKID&& i, string& d, int& l, int& c)
	:	data( d), Parseable(i, l, c) {}
	
	void Token::subReset() {
		data.clear();
	}
	
	
	
	// --- Private --- //
	
	Token Token::collect(string& b, int& l, int&& c) {
		TKID id = 
			  b == "SEQUENCE" ? KWSEQUENCE
			: b == "YIELD"    ? KWYIELD
			: b == "TRY"      ? KWTRY
			: b == "<-"       ? LARROW
			: b == "<->"      ? MARROW
			: b == "->"       ? RARROW
			: b[0] == ':'     ? COLON
			: b[0] == '_' || isalnum(b[0]) ? IDEN
			: UNKNOWN;
		
		bool numBucket;
		for(int i = 0; i < b.size() && (numBucket = isdigit(b[i])); i++);
		if(numBucket) id = NUM;

		return Token(move(id), b, l, c);
	}
	
	
	
	// --- Public --- //
	
	void Token::print() {
		cout << right
			<< setw(5) << lineIdx
			<< setw(5) << charIdx
			<< "  :  (" << setw(10);
			
		switch(ID) {
			case KWSEQUENCE : cout << "SEQUENCE" ; break ;
			case KWYIELD    : cout << "YIELD"    ; break ;
			case KWTRY      : cout << "TRY"      ; break ;
			case LARROW     : cout << "LARROW"   ; break ;
			case MARROW     : cout << "MARROW"   ; break ;
			case RARROW     : cout << "RARROW"   ; break ;
			case COLON      : cout << "COLON"    ; break ;
			case IDEN       : cout << "IDEN"     ; break ;
			case NUM        : cout << "NUM"      ; break ;
			case UNKNOWN    : cout << "UNKNOWN"  ; break ;
		}
		
		cout
			<< right << setw(5) << ID << left
			<< "  )  \"" << data << "\"\n";
	}
}