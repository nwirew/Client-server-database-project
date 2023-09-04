#pragma once

#include <cstring>
#include <netdb.h>
#include <signal.h>
#include <stdexcept>
#include <unordered_map>

#include "Token.h"
#include "Sequence.h"

namespace std {
	
	// --- Sequencer utilities --- //
	
	using cvec = vector<unsigned char>;
	enum MsgType : char {
		NOTHING, STOP, SKIP, SEND, DEFAULT, RENDEZVOUS, SIZECHECK, FINISHALERT
	};
	
	struct Partner {
		bool checkAgain = false;
		bool participating = false;
		int desc = -1;
	};
	
	// --- Sequencer class --- //
	
	class Sequencer : public Parseable {
		friend class SqrParser;
	private:
		int sub;
		int msg;
		int tries;
		string me;
		string choice;
		unordered_map<string,vector<unsigned char>> byteData;
		unordered_map<string,Partner> partners;
		unordered_map<string,Sequence> seqs;

		// Utility
		void subReset();
		
		bool checkData();
		bool checkMyData();
		bool handleOkay(string&);
		bool checkPartners();
		bool partnerOkay(const string&);
		bool checkPartnerData(Message* = nullptr);
		bool sizeOkay(Message&);
		
		Message prepareMessage(Message&);
		MsgType tryMessage(Message&);
		bool nextTry(MsgType);
		bool checkHistory(string&, bool);
		
		void checkYield();
		void sendMsg(int, cvec&, MsgType&& = DEFAULT, int&& = MSG_NOSIGNAL);
		MsgType recvMsg(int, cvec&, int&& = 0);
		
		void restart();
	public:
		static int debug;
		
		template<typename T>
		static cvec serialize(T&&);
		template<typename R>
		static R deserialize(cvec&);
		
		bool allowYield;
		
		Sequencer();
		
		void     establish();       // Identify data and party names
		void     choose(string&&);  // Choose a sequence to run
		void     iam(string&&);     // Identify self
		Partner& partner(string&&); // Get reference to comm. partner
		cvec&    data(string&&);    // Get reference to named message data
		int      run();             // Run the sequence
		void     stop();            // Prematurely stop the chosen sequence
	};
	
	
	
	/* --------------------------------------------------------------------- *\
	 * --- Template functions ---------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	
	
	template<typename T>
	cvec Sequencer::serialize(T&& thing) {
		cvec bucket;
		void* src = nullptr;
		int size = 0;
		
		if(typeid(T) == typeid(string)) {
			string& s = (string&)thing;
			src  = s.data();
			size = s.size();
		}
		else
		if(	typeid(T) == typeid(char)    || 
			typeid(T) == typeid(MsgType) ||
			typeid(T) == typeid(int)     ){
			src  = &thing;
			size = sizeof(thing)/sizeof(char);
		}
		else
			throw runtime_error("Sequencer serialize function could not "
			"determine the type of data to serialize");
		
		if(!src)
			throw logic_error("Sequencer serialize function could not acquire "
			"the address of the object to be serialized (likely not "
			"implemented)");
		else
		if(!size)
			throw logic_error("Sequencer serialize function could not "
			"determine the size of the object to be serialized (likely not "
			"implemented)");
		else {
			if(size < bucket.size())
				memset(bucket.data(), 0, bucket.size());
			bucket.resize(size);

			memcpy(bucket.data(), src, size);
		}
		
		return bucket;
	}
	
	template<typename R>
	R Sequencer::deserialize(cvec& serial) {
		R result;
		void* dst = nullptr;
		int size = 0;
		
		if(typeid(R) == typeid(string)) {
			string& s = (string&)result;

			if(serial.size() < s.size())
				memset(s.data(), 0, s.size());
			s.resize(serial.size());

			dst  = s.data();
			size = s.size();
		}
		else
		if(	typeid(R) == typeid(char)    || 
			typeid(R) == typeid(MsgType) ||
			typeid(R) == typeid(int)     ){
			dst  = &result;
			size = sizeof(result)/sizeof(char);
		}
		else
			throw runtime_error("Sequencer deserialize function could not "
			"determine the type of data to deserialize");
		
		if(!dst)
			throw logic_error("Sequencer deserialize function could not "
			"acquire the address of the object to be deserialized (likely not "
			"implemented)");
		else
		if(!size)
			throw logic_error("Sequencer deserialize function could not "
			"determine the size of the object to be deserialized (likely not "
			"implemented)");
		else
		if(size != serial.size())
			throw logic_error("Sequencer deserialize function found mismatch "
			"between buffer sizes (local: " +to_string(serial.size()) + ", "
			"foreign: " + to_string(size) + ")");
		else
			memcpy(dst, serial.data(), size);
		
		return result;
	}
}