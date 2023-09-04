/*\
 *        Author: Nathan Wilson Rew
 *  Date created: November 9, 2022
 *        Course: CSC 552
 *    Instructor: Dr. Spiegel
 *    Assignment: #3
 *      Due date: December 15, 2022
 *     File name: server.cpp
 *       Purpose: Defines common data structures for the server and client.
\*/

#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <semaphore.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>



namespace std {
	struct Record {
		unsigned char state[       2+1 ] = { 0 };
		unsigned char city[       16+1 ] = { 0 };
		unsigned char applicant[  35+1 ] = { 0 };
		unsigned char licenseNum[ 13+1 ] = { 0 };
		unsigned int  docket             =   0  ;
		unsigned char region             =   0  ;
		unsigned char date[        8+1 ] = { 0 };
		unsigned char type               =   0  ;
		
		vector<unsigned char> serialize() {
			vector<unsigned char> bytes;
			bytes.resize(sizeof(Record));
			memcpy(bytes.data(), this, sizeof(Record));
			return bytes;
		}
		
		void deserialize(vector<unsigned char>& bytes) {
			if(bytes.size() >= sizeof(Record))
				memcpy(this, bytes.data(), sizeof(Record));
			else
				throw runtime_error("Record deserialize function's byte "
				"vector parameter does not contain enough bytes");
		}
	};
	ostream& operator<<(ofstream&, const Record&);
	ostream& operator<<(fstream&,  const Record&);
	ostream& operator<<(ostream&,  const Record&);
	istream& operator>>(istream&,  Record&);
}



template<typename T>
class Shr {
public:
	Shr(std::string);
	~Shr();
	
	static Shr* Ptr;
	static constexpr int Tick = 100000;
	
	// Shared memory gets several variables:
	// - File streams
	// - Server or client data
	std::fstream logFile;
	T data;
};



// Local server data class.
class Srvr {
public:
	Srvr();
	~Srvr();
	
	// static constexpr int MaxClients = 1024;
	
	static std::string IP;
	static int Port;
	static int Sock;
	static int Conn;
	
	static const std::string binFileName;
	std::fstream binFile;
	
	int   numServers         ;
	int   numReadersLogFile  ;
	int   numReadersBinFile  ;
	pid_t parentPID          ;
	bool  serShutdownTrigger ;
	bool  serShutdown        ;
	
	sem_t semNumServers   ;
	sem_t semSigintGuard  ;

	sem_t semOrderLogFile      ;
	sem_t semNumReadersLogFile ;
	sem_t semWriteLogFile      ;
	
	sem_t semOrderBinFile      ;
	sem_t semNumReadersBinFile ;
	sem_t semWriteBinFile      ;
	
	void beginReadBinFile();
	void endReadBinFile();
	void beginWriteBinFile();
	void endWriteBinFile();
};



struct ClientSnapshot {
	using NowType = std::chrono::time_point<std::chrono::system_clock>;
	
	bool occupied;
	pid_t pid;
	NowType lastMsgTime;
	int numCommands;
	
	ClientSnapshot(bool = false, pid_t = -1, NowType = {}, int = 0);
	~ClientSnapshot();
};

// Shared client data class.
class Clnt {
private:
	static constexpr int maxClients = 1024;
public:
	static int Port;
	static int Sock;
	static int Conn;
	
	static const std::string UserPrefix ;

	// static const std::string SemNameOrderMem   ; // ???
	static const std::string SemNameNumClients ;
	
	static const std::string SemNameOrderMem      ;
	static const std::string SemNameNumReadersMem ;
	static const std::string SemNameWriteMem      ;

	static const std::string SemNameOrderLogFile      ;
	static const std::string SemNameNumReadersLogFile ;
	static const std::string SemNameWriteLogFile      ;
	
	// Named semaphores are managed by the OS, not stored in shared memory.
	// Program is given a different address every time.
	// static sem_t* semOrderMem   ; // ???
	static sem_t* semNumClients ;
	
	static sem_t* semOrderMem      ;
	static sem_t* semNumReadersMem ;
	static sem_t* semWriteMem      ;

	static sem_t* semOrderLogFile      ;
	static sem_t* semNumReadersLogFile ;
	static sem_t* semWriteLogFile      ;
	
	static ClientSnapshot* mySnap;
	
	ClientSnapshot snapshots[maxClients];
	int numClients        ;
	int lastOccupied      ;
	int numReadersLogFile ;
	int numReadersMem     ;
	
	Clnt();
	~Clnt();
	
	void clientEnter();
	void clientUpdateInfo();
	void clientExit();
	
	void memSem(const char[2]);
	void logSem(const char[2]);
	
	// https://www.youtube.com/watch?v=kSWfushlvB8&t=2716s
};