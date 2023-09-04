/*\
 *        Author: Nathan Wilson Rew
 *  Date created: November 9, 2022
 *        Course: CSC 552
 *    Instructor: Dr. Spiegel
 *    Assignment: #3
 *      Due date: December 15, 2022
 *     File name: server.cpp
 *       Purpose: Implements common data structures for the server and client.
\*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include "Shr.h"

namespace std {
	ostream& operator<<(ofstream& ofs, const Record& R) {
		return ofs.write((char*)&R, sizeof(R));
	}

	ostream& operator<<(fstream& fs, const Record& R) {
		return fs.write((char*)&R, sizeof(R));
	}

	ostream& operator<<(ostream& os, const Record& R) {
		return os
			<<  setw( 2)  <<  left   <<  R.state             <<  ", " 
			<<  setw(16)  <<  left   <<  R.city              <<  ", "
			<<  setw(35)  <<  left   <<  R.applicant         <<  ", "
			<<  setw(13)  <<  left   <<  R.licenseNum        <<  ", "
			<<  setw(10)  <<  right  <<  R.docket            <<  ", "
			<<  setw( 3)  <<  right  <<  (unsigned)R.region  <<  ", "
			<<  setw( 8)  <<             R.date              <<  ", "
			<<  setw( 3)  <<             (unsigned)R.type    <<  endl;
	}
	
	istream& operator>>(istream& is,  Record& R) {
		return is.read((char*)&R, sizeof(R));
	}
}



template<typename T>
Shr<T>* Shr<T>::Ptr = nullptr;

template<typename T>
Shr<T>::Shr(std::string logFileName)
  : logFile(logFileName, std::ios::in | std::ios::out | std::ios::trunc),
    data() {}

template<typename T>
Shr<T>::~Shr() {
	data.~T();
}



std::string Srvr::IP;
int Srvr::Port = 2008;
int Srvr::Sock = -1  ;
int Srvr::Conn = -1  ;

using ios = std::ios;
Srvr::Srvr() :
serShutdownTrigger(false),
serShutdown(false),
parentPID(getpid()),
binFile("data.bin", ios::in | ios::out | ios::binary | ios::ate) {
	sem_init(&semNumServers,   true, true);
	sem_init(&semSigintGuard,  true, true);

	// Set up server log file semaphores
	sem_init(&semOrderLogFile,      true, true);
	sem_init(&semNumReadersLogFile, true, true);
	sem_init(&semWriteLogFile,      true, true);
	
	// Set up binary data file semaphores
	sem_init(&semOrderBinFile,      true, true);
	sem_init(&semNumReadersBinFile, true, true);
	sem_init(&semWriteBinFile,      true, true);
}

Srvr::~Srvr() {
	sem_destroy(&semNumServers  );
	sem_destroy(&semSigintGuard );
	
	// Release server log file semaphores
	sem_destroy(&semOrderLogFile     );
	sem_destroy(&semNumReadersLogFile);
	sem_destroy(&semWriteLogFile     );
	
	// Release binary data file semaphores
	sem_destroy(&semOrderBinFile     );
	sem_destroy(&semNumReadersBinFile);
	sem_destroy(&semWriteBinFile     );
}

void Srvr::beginReadBinFile() {
	sem_wait(&semOrderBinFile);
	sem_wait(&semNumReadersBinFile);
	if(++numReadersBinFile == 1)
		sem_wait(&semWriteBinFile);
	sem_post(&semOrderBinFile);
	sem_post(&semNumReadersBinFile);
}

void Srvr::endReadBinFile() {
	sem_wait(&semNumReadersBinFile);
	if(--numReadersBinFile == 0)
		sem_post(&semWriteBinFile);
	sem_post(&semNumReadersBinFile);
}

void Srvr::beginWriteBinFile() {
	sem_wait(&semOrderBinFile);
	sem_wait(&semWriteBinFile);
	sem_post(&semOrderBinFile);
}

void Srvr::endWriteBinFile() {
	sem_post(&semWriteBinFile);
}



ClientSnapshot::ClientSnapshot(bool o, pid_t p, NowType t, int n)
  : occupied{o}, pid{p}, lastMsgTime{t}, numCommands{n} {}

ClientSnapshot::~ClientSnapshot()
  { occupied = false; }



const std::string Clnt::UserPrefix = std::string{'/'} + getlogin();

// Client shared memory semaphore names
const std::string Clnt::SemNameNumClients
	= Clnt::UserPrefix + "numclients";

const std::string Clnt::SemNameOrderMem
	= Clnt::UserPrefix + "ordermem"     ;
const std::string Clnt::SemNameNumReadersMem
	= Clnt::UserPrefix + "numreadersmem";
const std::string Clnt::SemNameWriteMem
	= Clnt::UserPrefix + "writemem"     ;

// Client log file semaphore names
const std::string Clnt::SemNameOrderLogFile
	= Clnt::UserPrefix + "orderlogfile"     ;
const std::string Clnt::SemNameNumReadersLogFile
	= Clnt::UserPrefix + "numreaderslogfile";
const std::string Clnt::SemNameWriteLogFile
	= Clnt::UserPrefix + "writelogfile"     ;

// Client shared memory semaphores
sem_t* Clnt::semNumClients = nullptr ;

sem_t* Clnt::semOrderMem      = nullptr ;
sem_t* Clnt::semNumReadersMem = nullptr ;
sem_t* Clnt::semWriteMem      = nullptr ;

// Client log file semaphores
sem_t* Clnt::semOrderLogFile      = nullptr ;
sem_t* Clnt::semNumReadersLogFile = nullptr ;
sem_t* Clnt::semWriteLogFile      = nullptr ;

int Clnt::Port = 2008 ;
int Clnt::Sock = -1   ;
int Clnt::Conn = -1   ;
ClientSnapshot* Clnt::mySnap = nullptr;

Clnt::Clnt()
  : numClients{0}, lastOccupied{numClients-1}, snapshots{},
    numReadersLogFile{0}, numReadersMem{0} {}

Clnt::~Clnt() {
	using namespace std;
	
	sem_unlink(Clnt::SemNameNumClients  .c_str());
	
	sem_unlink(Clnt::SemNameOrderMem     .c_str());
	sem_unlink(Clnt::SemNameNumReadersMem.c_str());
	sem_unlink(Clnt::SemNameWriteMem     .c_str());
	
	sem_unlink(Clnt::SemNameOrderLogFile     .c_str());
	sem_unlink(Clnt::SemNameNumReadersLogFile.c_str());
	sem_unlink(Clnt::SemNameWriteLogFile     .c_str());
}

void Clnt::clientEnter() {
	using namespace std;
	
	memSem("pw");  // CS begin
	
	// 1: Early exit if this device already hosts the maximum amount of clients
	if(numClients >= maxClients) {
		memSem("vw");
		// clientExit() ?
		throw runtime_error("Too many connected clients, please wait for "
		"another to finish");
	}
	
	int idx;
	if(lastOccupied+1 == numClients)
		idx = ++lastOccupied;
	else {
		idx = 0;
		while(snapshots[idx].occupied && idx <= lastOccupied) idx++;
		while(idx > lastOccupied) lastOccupied++;
	}
	
	mySnap = new(snapshots + idx) ClientSnapshot
		(true, getpid(), chrono::system_clock::now(), 0);
	
	numClients++;
	memSem("vw");  // CS end
}

void Clnt::clientUpdateInfo() {
	memSem("pw");
	mySnap->lastMsgTime = std::chrono::system_clock::now();
	mySnap->numCommands++;
	memSem("vw");
}

void Clnt::clientExit() {
	memSem("pw");
	if(mySnap) {
		numClients--;
		
		mySnap->~ClientSnapshot();
		
		if(mySnap == snapshots + lastOccupied) {
			while(!snapshots[lastOccupied].occupied && lastOccupied >= 0)
				lastOccupied--;
		}
		
		mySnap = nullptr;
	}
	memSem("vw");
}

void Clnt::memSem(const char OpRole[2]) {
	if(OpRole[0] == 'p') {
		if(OpRole[1] == 'r') {           // Acquire as reader
			sem_wait(semOrderMem);
			sem_wait(semNumReadersMem);
			if(numReadersMem++ == 0)
				sem_wait(semWriteMem);
			sem_post(semOrderMem);
			sem_post(semNumReadersMem);
		} else
		if(OpRole[1] == 'w')             // Acquire as writer
			sem_wait(semOrderMem),
			sem_wait(semWriteMem),
			sem_post(semOrderMem);
	} else
	if(OpRole[0] == 'v') {
		if(OpRole[1] == 'r') {           // Release as reader
			sem_wait(semOrderMem);
			if(--numReadersMem == 0)
				sem_post(semWriteMem);
			sem_post(semOrderMem);
		} else
		if(OpRole[1] == 'w')             // Release as writer
			sem_post(semWriteMem);
	}
}

void Clnt::logSem(const char OpRole[2]) {
	if(OpRole[0] == 'p') {
		if(OpRole[1] == 'r') {           // Acquire as reader
			sem_wait(semOrderLogFile);
			sem_wait(semNumReadersLogFile);
			if(numReadersLogFile++ == 0)
				sem_wait(semWriteLogFile);
			sem_post(semOrderLogFile);
			sem_post(semNumReadersLogFile);
		} else
		if(OpRole[1] == 'w')             // Acquire as writer
			sem_wait(semOrderLogFile),
			sem_wait(semWriteLogFile),
			sem_post(semOrderLogFile);
	} else
	if(OpRole[0] == 'v') {
		if(OpRole[1] == 'r') {           // Release as reader
			sem_wait(semOrderLogFile);
			if(--numReadersLogFile == 0)
				sem_post(semWriteLogFile);
			sem_post(semOrderLogFile);
		} else
		if(OpRole[1] == 'w')             // Release as writer
			sem_post(semWriteLogFile);
	}
}



template class Shr<Srvr>;
template class Shr<Clnt>;