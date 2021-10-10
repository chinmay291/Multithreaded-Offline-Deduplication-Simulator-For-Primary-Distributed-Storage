class Server{
	public:
		class VM{
			public:
				FILE* inputFile;	//Contains a list of memory addresses that the VM wants to access
				int id;
		};

		class Dom0{
			public:
				queue<pair<long long, long long>> unreferencedQueue;	// Stores (logical address, previous physical address)
				queue<long long> dirtyQueue;
				queue<long long> freeQueue;
				unordered_map<long long, long long> logicalToPhysicalMapping;    // maps server logical address : physical address
				unordered_set<long long> setOfCOWAddresses;		// Stores the set of logical addresses that are CoW
		};

		vector<VM> VMOnServer;		
		Dom0 dom0;
		int id;							//ID of the server
		int currVMRunningID = 0;		//ID of the VM that is currently running	
		FILE* outputFile;		// Output of print statements goes to this file

		Server(int serverId, int numVMs){
			id = serverId;
			for(int i = 0; i < numVMs; i++){
				VM vm;
				VMOnServer.push_back(vm);
			}
			string outputFileNameStr = "./output_" + to_string(serverId) + ".txt";
			const char* outputFileName = outputFileNameStr.c_str();
			outputFile = fopen(outputFileName, "w");
		}

		// Generates a random signature and writes it to the block with the given logical address
		bool write(long long logicalAddress, string signature);	

		// Aliases the block at the front of the dirty queue with a duplicate if one is available
		bool runDuplicateFinderOnNextBlock();	

		// Reclaims the block at the front of the unreferences queue and adds it to the free queue if it has zero references
		bool runGarbageCollectionOnNextBlock();

		// Checks whether the block is a COW block and returns true if it is and false otherwise
		bool checkCOWStatus(long long logicalAddress);

		// Sets the COW status of the given block according to COWvalue
		bool setCOWStatus(long long logicalAddress, bool COWvalue);

		// Returns the physical address corresponding to the logical address and -1 if there is no mapping exists for it
		long long convertLogicalToPhysical(long long logicalAddress);

		// Updates an entry in the logical to physical mapping and returns true
		bool updateLogicalToPhysicalMapping(long long logicalAddress, long long freeBlockAddress);

		// Returns physical address of the next block from the free queue if one is available. Otherwise requests for storage allocation
		// from the extent server and then returns a free block
		long long getBlockFromFreeQueue();

		// Returns logical address of the block at the front of the dirty queue or -1 if the dirty queue is empty
		long long getBlockFromDirtyQueue();

		// Returns logical address and previous physical address of the block at the front of the unreferenced queue or -1 if the unreferenced queue is empty
		pair<long long, long long> getBlockFromUnreferencedQueue();

		// Inserts the given logical address into the unreferenced queue
		bool insertIntoUnreferencedQueue(long long logicalAddress, long long physicalAddress);

		// Inserts the given logical address into the dirty queue
		bool insertIntoDirtyQueue(long long logicalAddress);

		// Inserts the given physical address into the free queue
		bool insertIntoFreeQueue(long long physicalAddress);

		// Returns the id of the next VM to schedule
		int scheduleNextVM();

		long long readNextAddressFromInputFile(int vmID);

		string readNextSignatureFromInputFile(int vmID);

		char readNextOperationTypeFromInputFile(int vmID);

		bool processRead(long long logicalAddress);

		bool startServerExecution();

		bool initializeVM(int id, char* inputFilePath);
};