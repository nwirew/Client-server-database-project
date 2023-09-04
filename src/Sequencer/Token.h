#pragma once

#include "Parseable.h"

namespace std {
	class Token : public Parseable {
	private:
		void subReset();
	public:
		static Token collect(string&, int&, int&&);
		
		string data;
		
		Token(/*-----------------------*/);
		Token(TKID&&, string&, int&, int&);
		
		void print();
	};
}