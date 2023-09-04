#pragma once

#include "Parseable.h"
#include "Subsequence.h"

namespace std {
	class Sequence : public Parseable {
	private:
		void subReset();
	public:
		vector<Subsequence> subs;
		
		Sequence();
		
		template<typename T>
		void attach(string&&, T&);
	};
}