/*\
 *        Author: Nathan Wilson Rew
 *  Date created: November 9, 2022
 *        Course: CSC 552
 *    Instructor: Dr. Spiegel
 *    Assignment: #3
 *      Due date: December 15, 2022
 *     File name: client.cpp
 *       Purpose: Interact with the server application to demonstrate
 *                semaphores, shared memory, and sockets.
\*/

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <signal.h>
#include <sys/mman.h>

#include "Shr.h"
#include "Sequencer/SqrParser.h"

/*\
 *  Global variables and function prototypes
\*/
namespace std {
	int  setupSocket(sockaddr_in&, int);
	bool setupConnection();
	template<typename ...SIGS>
	void setupSigHandler(int, __sighandler_t, SIGS...);
	bool setupSharedMemory();
	void talkToServer();
	
	// Data functions
	void displayRecord();
	void modifyRecord();
	void createRecord();
	
	void changeRecordProperty(Record&, int);
	
	// Utility functions
	void displayCliLog();
	void displayActiveClients();
	void displaySerLog();
	
	void cleanupSharedMemory();
	void cleanupConnection();
	
	void departServer(int);
	
	template<typename T>
	T cinget();
	
	template<typename ...Args>
	void cliLog(Args... args);
	
	Sequencer sqr;
}

int main(int argc, char** argv) {
    using namespace std;
	
	Sequencer::debug = 0;
	sqr = SqrParser::create("sequence/RecordStuff.sequence");

	signal(SIGINT, SIG_IGN);
	
    if(!setupConnection()) return 1;
    if(setupSharedMemory()) {
		cliLog("--- BEGIN CLIENT ", getpid(), " LOG ---");
		talkToServer();
		cliLog("--- END CLIENT ", getpid(), " LOG ---");
    }
    cleanupSharedMemory();
    cleanupConnection();
	
    return 0;
}


/*\
 *  Function implementations
\*/
namespace std {
	int setupSocket(sockaddr_in& sin, int reqPort = -1) {
		struct hostent*  hinf = gethostbyname("acad.kutztown.edu");
		struct protoent* pinf = getprotobyname("tcp");
		int sockFD = socket(AF_INET, SOCK_STREAM, pinf->p_proto);
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(reqPort);
		memcpy(&sin.sin_addr, hinf->h_addr, hinf->h_length);
		return sockFD;
	}
	
	
	
	bool setupConnection() {
		// 1: Get the server connection on the "well known" port
		struct sockaddr_in sin;
		Clnt::Sock = setupSocket(sin, Clnt::Port);
		
		// 2: Poke the server for up to 5 seconds; exit if no response
		cout << "Establishing server connection.\n";
		int tries = 0, max = 5 * 1000000/Shr<Clnt>::Tick;
		while(Clnt::Conn == -1 && tries < max) {
			Clnt::Conn = connect(Clnt::Sock, (struct sockaddr*)&sin, sizeof(sin));
			if(Clnt::Conn == -1) {
				usleep(Shr<Clnt>::Tick);
				cout
					<< "Attempt " << ++tries << " of " << max << " failed, "
					<< (tries < max ? "trying again..." : "stopping.") << endl;
			}
		}
		
		// 3: If a response came through, it contains the service port.
		if(!Clnt::Conn) {
			cout << "Client " << getpid() << " connected to server on port "
			<< Clnt::Port << "!\n";
			
			sqr.choose("TellPort");
			sqr.iam("CLIENT");
			sqr.partner("SERVER").desc = Clnt::Sock;
			sqr.data("PortNum").resize(sizeof(Clnt::Port));
			sqr.run();
			
			int servicePort = Sequencer::deserialize<int>(sqr.data("PortNum"));
			
			cout << "Client " << getpid() << " acquired service port "
			<< servicePort << "!\n";
			
			cout << "Closing connection port " << Clnt::Port << ".\n";
			close(Clnt::Port);
			
			Clnt::Port = servicePort;
		}
		else
			cout << "Client " << getpid() << " could not connect to server.\n";
		
		// Conn == return val of connect(), -1 on error and 0 on success
		return !Clnt::Conn;
	}
	
	
	
	bool setupSharedMemory() {
		constexpr int rdwr = 0666;
		
		bool iAmFirst;
		int fd;
		Shr<Clnt>* shr;
		
		// 1: Open named semaphores. Open the final semaphore without O_CREAT
		//    after waiting on any of the previous ones to check if this is the
		//    first client. sem_open returns an error value if it is closed.
		Clnt::semOrderLogFile      =
			sem_open(Clnt::SemNameOrderLogFile     .c_str(), O_CREAT, rdwr, 1);
		Clnt::semNumReadersLogFile =
			sem_open(Clnt::SemNameNumReadersLogFile.c_str(), O_CREAT, rdwr, 1);
		Clnt::semWriteLogFile      =
			sem_open(Clnt::SemNameWriteLogFile     .c_str(), O_CREAT, rdwr, 1);
		
		Clnt::semNumClients        =
			sem_open(Clnt::SemNameNumClients       .c_str(), O_CREAT, rdwr, 1);
		sem_wait(Clnt::semNumClients);
		
		Clnt::semOrderMem          =
			sem_open(Clnt::SemNameOrderMem         .c_str(), O_CREAT, rdwr, 1);
		Clnt::semNumReadersMem     =
			sem_open(Clnt::SemNameNumReadersMem    .c_str(), O_CREAT, rdwr, 1);
		Clnt::semWriteMem          = 
			sem_open(Clnt::SemNameWriteMem         .c_str(), 0,       rdwr, 1);
		
		// 2: Force last semaphore open and get shared memory backing file
		if(iAmFirst = (Clnt::semWriteMem == SEM_FAILED)) {
			Clnt::semWriteMem =
				sem_open(Clnt::SemNameWriteMem     .c_str(), O_CREAT, rdwr, 1);
			fd = open(".ClientShared.mem", O_RDWR | O_CREAT | O_TRUNC);
			ftruncate(fd, sizeof(Shr<Clnt>));
			system("chmod 666 .ClientShared.mem");
		}
		else
			fd = open(".ClientShared.mem", O_RDWR);

		// 3: Obtain shared memory mapping
		Shr<Clnt>::Ptr = (Shr<Clnt>*)mmap(
			nullptr,  sizeof(Shr<Clnt>),
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0
		);
		
		close(fd);

		// 4: First client needs to wipe and initialize shared memory
		if(iAmFirst) {
			memset(Shr<Clnt>::Ptr, 0, sizeof(Shr<Clnt>));
			Shr<Clnt>::Ptr = new (Shr<Clnt>::Ptr) Shr<Clnt>("cli.log");
			cliLog("Client ", getpid(), " has set up shared memory.");
		}

		// 5: Make an entry for this client in the bookkeeping section of
		//    shared memory
		Shr<Clnt>::Ptr->data.clientEnter();

		// 6: Release semaphore and let other clients through
		sem_post(Clnt::semNumClients);

		return true;
	}
	
	
	
	void talkToServer() {
		MsgType response;
		char command = '?';
		
		while(command) {
			cout
				<< "Main menu:\n"
				<< "- Data:\n"
				<< "  - D: Display record\n"
				<< "  - M: Modify record\n"
				<< "  - N: New record\n"
				<< "- Utility:\n"
				<< "  - C: Show clients' log\n"
				<< "  - P: Print local clients' info\n"
				<< "  - S: Show server log\n"
				<< "  - E: Exit\n"
				<< " > ";
			
			command = tolower(cinget<char>());

			// Send requested command and wait for server's decision
			sqr.choose("Command");
			sqr.data("CommandID") = Sequencer::serialize(command);
			sqr.data("Decision") = Sequencer::serialize(response);
			sqr.run();
			response = Sequencer::deserialize<MsgType>(sqr.data("Decision"));
			
			if(response == STOP) {
				cliLog("Server has exited");
				command = 0;
			}
			else
				Shr<Clnt>::Ptr->data.clientUpdateInfo();
			
			switch(command) {
				// Data
				case 'd': displayRecord();        break;
				case 'm': modifyRecord();         break;
				case 'n': createRecord();         break;
				// Utility
				case 'c': displayCliLog();        break;
				case 'p': displayActiveClients(); break;
				case 's': displaySerLog();        break;
				case 'e': command = 0;            break;
			}
			
			if(response == STOP)
				command = 0;
		}
		
		close(Clnt::Sock);
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	** --- Data functions -------------------------------------------------- **
	\* --------------------------------------------------------------------- */
	
	
	
	void displayRecord() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to view a record.");
		
		// 1: Get the number of records
		sqr.choose("ChooseIndex");
		sqr.data("RecordCount").resize(sizeof(int));
		sqr.data("RecordChoice").resize(sizeof(int));
		sqr.run();
		
		int numRecs = Sequencer::deserialize<int>(sqr.data("RecordCount"));
		int choice = 0;
		
		// 2: Get the record index
		while((choice < 1 || numRecs < choice) && choice != -999) {
			cout
				<< "There are " << numRecs << " records. "
				<< "Which one would you like to view? "
				<< "(-999 = All)\n > ";
			choice = cinget<int>();
		}
		
		sqr.data("RecordChoice") = Sequencer::serialize(choice);
		sqr.run(); // End ChooseIndex
		
		// 3: Get the requested record
		sqr.choose("SendRecord");
		sqr.iam("RECEIVER");
		sqr.partner("SENDER") = sqr.partner("SERVER");
		sqr.data("Record").resize(sizeof(Record));
		Record rec;
		
		// 4: Display the requested record(s)
		if(choice == -999) {
			for(int i = 0; i < numRecs; i++) {
				sqr.run();
				rec.deserialize(sqr.data("Record"));
				cout << rec;
			}
		}
		else {
			sqr.run();
			rec.deserialize(sqr.data("Record"));
			cout << rec;
		}
		
		// 5: Set my sequencer role back to my chosen default
		sqr.iam("CLIENT");
		cout << endl;
	}
	
	
	
	void modifyRecord() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to modify a record.");
		
		// 1: Choose the record
		sqr.choose("ModifyRecord");
		sqr.data("RecordCount").resize(sizeof(int));
		sqr.data("RecordChoice").resize(sizeof(int));
		sqr.data("Record").resize(sizeof(Record));
		sqr.run();
		
		int numRecs = Sequencer::deserialize<int>(sqr.data("RecordCount"));
		int choice = 0;
		Record rec;
		
		while(choice < 1 || numRecs < choice) {
			cout
				<< "There are " << numRecs << " records. "
				<< "Which one would you like to modify?\n"
				<< " > ";
			choice = cinget<int>();
		}
		
		sqr.data("RecordChoice") = Sequencer::serialize(choice);
		sqr.run();
		rec.deserialize(sqr.data("Record"));
		
		do {
			choice = 0;
			
			// 2: Choose the record field to modify
			while(choice < 1 || 9 < choice) {
				cout
					<< "Choose a record property to change:\n"
					<< "- 1: State ............ " << rec.state       << endl
					<< "- 2: City ............. " << rec.city        << endl
					<< "- 3: Applicant ........ " << rec.applicant   << endl
					<< "- 4: License number ... " << rec.licenseNum  << endl
					<< "- 5: Docket number .... " << rec.docket      << endl
					<< "- 6: Region ID ........ " << (int)rec.region << endl
					<< "- 7: Date ............. " << rec.date        << endl
					<< "- 8: Type ............. " << (int)rec.type   << endl
					<< "- 9: Finish and commit changes\n > ";
				choice = cinget<int>();
			}
			
			// 3: Get a new value for the chosen field
			if(choice != 9) changeRecordProperty(rec, choice);
			
		} while(choice != 9);
		
		// 4: Send the modified record
		sqr.data("Record") = rec.serialize();
		sqr.run();
		cout << endl;
	}
	
	
	
	void createRecord() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to create a new record.");
		
		Record rec;
		
		// 1: Set all record properties
		for(int i = 1; i <= 8; i++)
			changeRecordProperty(rec, i);
		
		// 2: Send the record to the server
		sqr.choose("SendRecord");
		sqr.iam("SENDER");
		sqr.partner("RECEIVER") = sqr.partner("SERVER");
		sqr.data("Record") = rec.serialize();
		sqr.run();
		
		// 3: Set my sequencer role back to my chosen default
		sqr.iam("CLIENT");
		cout << endl;
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	** --- Utility functions ----------------------------------------------- **
	\* --------------------------------------------------------------------- */
	
	
	
	void displayCliLog() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to view a the clients' log.");
		
		Clnt& data = Shr<Clnt>::Ptr->data;
		
		data.logSem("pr");  // CS begin
		cout << "\nClient log file:\n";
		cout << ifstream("cli.log").rdbuf();
		cout << endl;
		data.logSem("vr");  // CS end
		cout << endl;
	}
	
	
	
	void displayActiveClients() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to view local client info.");
		
		using namespace std::chrono;
		Clnt& data = Shr<Clnt>::Ptr->data;
		
		data.memSem("pr");  // CS begin
		
		cout << "Clients on this machine:\n"
			"- PID       Commands  Latest\n";
		for(int i = 0; i <= data.lastOccupied; i++) {
			ClientSnapshot& snap = data.snapshots[i];
			if(snap.occupied) {
				duration time = snap.lastMsgTime.time_since_epoch();
				duration hours = duration_cast<chrono::hours>(time);
				duration minutes = duration_cast<chrono::minutes>(time);
				duration seconds = duration_cast<chrono::seconds>(time);
				duration ms = duration_cast<chrono::milliseconds>(time);
				
				cout << "- "
					<< left << setw(10) << snap.pid
					<< left << setw(10) << snap.numCommands
					<< left << setfill('0')
						<< setw(2) << hours.count() % 24 << ':'
						<< setw(2) << minutes.count() % 60 << ':'
						<< setw(2) << seconds.count() % 60 << ':'
						<< setw(3) << ms.count() % 1000
					<< setfill(' ')
					<< endl;
			}
		}
		data.memSem("vr");  // CS end
		cout << endl;
	}
	
	
	
	void displaySerLog() {
		cout << endl;
		cliLog("Client ", getpid(), " requested to view the server log.");
		
		Clnt& data = Shr<Clnt>::Ptr->data;
		int chunkSize = 128;
		string bucket {};
		
		sqr.choose("SendLogFile");
		sqr.iam("RECEIVER");
		sqr.partner("SENDER") = sqr.partner("SERVER");
		sqr.data("LogFileChunk").resize(chunkSize+1);
		
		vector<unsigned char>& chunk = sqr.data("LogFileChunk");
		
		cout << "\nServer log file:\n";
		do {
			sqr.run();
			bucket.resize(chunk.back());
			memcpy(bucket.data(), chunk.data(), chunk.back());
			cout << bucket;
		} while(sqr.data("LogFileChunk").back() == chunkSize);
		cout << endl;
		
		sqr.iam("CLIENT");
		cout << endl;
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	** --- Testing functions ----------------------------------------------- **
	\* --------------------------------------------------------------------- */
	
	
	
	
	void changeRecordProperty(Record& rec, int propID) {
		bool ok = false;
		string newData;
		void* dataLoc;
		int sizeLimit;
		
		while(!ok) {
			newData.clear();
			cout << "Please enter the new ";
			
			switch(propID) {
				case 1: {
					sizeLimit = 2;
					dataLoc = rec.state;
					cout << "state (2-letter abbreviation):\n > ";
					break;
				}
				case 2: {
					sizeLimit = 16;
					dataLoc = rec.city;
					cout << "city (truncated to 16 characters):\n > ";
					break;
				}
				case 3: {
					sizeLimit = 35;
					dataLoc = rec.applicant;
					cout << "applicant name (truncated to 35 characters):\n > ";
					break;
				}
				case 4: {
					sizeLimit = 13;
					dataLoc = rec.licenseNum;
					cout << "license number (truncated to 13 characters):\n > ";
					break;
				}
				case 5: {
					sizeLimit = sizeof(unsigned int);
					dataLoc = &rec.docket;
					cout << "docket number (positive number):\n > ";
					newData.resize(sizeLimit);
					unsigned int newDocket = cinget<unsigned int>();
					memcpy(newData.data(), &newDocket, sizeLimit);
					break;
				}
				case 6: {
					sizeLimit = sizeof(char);
					dataLoc = &rec.region;
					cout << "region ID (0-255):\n > ";
					newData.resize(sizeLimit);
					unsigned char newRegion =
						(unsigned char)cinget<int>();
					memcpy(newData.data(), &newRegion, sizeLimit);
					break;
				}
				case 7: {
					sizeLimit = 8;
					dataLoc = rec.date;
					cout << "date (mm/dd/yy format):\n > ";
					break;
				}
				case 8: {
					sizeLimit = sizeof(char);
					dataLoc = &rec.type;
					cout << "type (0-255):\n > ";
					newData.resize(sizeLimit);
					unsigned char newType =
						(unsigned char)cinget<int>();
					memcpy(newData.data(), &newType, sizeLimit);
					break;
				}
			}
			
			// Only empty if this is a char-array property
			if(newData.empty()) getline(cin, newData);
			ok = newData.size() <= sizeLimit;
			if(ok)
				memset(dataLoc, 0, sizeLimit),
				memcpy(dataLoc, newData.data(), newData.size());
		}
	}
	
	
	
	void cleanupSharedMemory() {
		if(Shr<Clnt>::Ptr) {
			Clnt& data = Shr<Clnt>::Ptr->data;
			Shr<Clnt>* shr = Shr<Clnt>::Ptr;
			
			// Obtain the shmem semaphore and update the number of clients
			sem_wait(Clnt::semNumClients);
			Shr<Clnt>::Ptr->data.clientExit();
			
			// If I am last client, wipe the shared memory file.
			if(!data.numClients) {
				cliLog(getpid(), " is the last client. Cleaning up shared "
				"resources...");
				shr->data.~Clnt();
				munmap(Shr<Clnt>::Ptr, sizeof(Shr<Clnt>));
			}
			else
				cliLog(getpid(), " is not the last client.");
			
			sem_post(Clnt::semNumClients);
			
			// Release this client's semaphore resources
			sem_close(Clnt::semNumClients);
			
			sem_close(Clnt::semOrderMem);
			sem_close(Clnt::semNumReadersMem);
			sem_close(Clnt::semWriteMem);
			
			sem_close(Clnt::semOrderLogFile);
			sem_close(Clnt::semNumReadersLogFile);
			sem_close(Clnt::semWriteLogFile);
		}
	}
	
	
	
	void cleanupConnection() {
		// Msg().send(Clnt::Sock, "client shutdown", 15, Msg::FINISH);
		// TODO
		close(Clnt::Sock);
		close(Clnt::Conn);
		close(Clnt::Port);
	}
	
	
	
	void departServer(int) {
		using namespace std;
		
		cliLog("\n--> Client ", getpid(), " received interrupt signal.");
		
		// 1: Get the shutdown response.
		char choice;
		do {
			cout << "Shut down client " << getpid() << "? (y/n)\n > ";
			choice = tolower(cinget<char>());
		}
		while (choice != 'y' && choice != 'n');
		
		// 2: Close the port if requested
		if(choice == 'y') {
			cleanupConnection();
			cliLog("<-- Client ", getpid(), " is beginning shutdown.");
		}
		else
			cliLog("<-- Client ", getpid(), " is resuming...");
	}
	
	
	
	template<typename T>
	T cinget() {
		T dest;
		cin >> dest;
		if(cin.fail()) cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		return dest;
	}
	
	
	
	template<typename ...Args>
	void cliLog(Args... args) {
		using namespace std;
		
		// Have to block signals until the semaphore is released because:
		// - Don't want to leave semaphores locked without a way to unlock them
		sigset_t blocked;
		sigemptyset(&blocked);
		sigaddset(&blocked, SIGINT);
		
		Shr<Clnt>* Ptr = Shr<Clnt>::Ptr;

		sigprocmask(SIG_BLOCK, &blocked, nullptr);
		
		Ptr->data.logSem("pw");
		fstream LOGFILE("cli.log", ios::in | ios::out | ios::app);
		(LOGFILE << ... << args) << endl;
		(cout    << ... << args) << endl;
		LOGFILE.close();
		Ptr->data.logSem("vw");
		
		sigprocmask(SIG_UNBLOCK, &blocked, nullptr);
	}
}