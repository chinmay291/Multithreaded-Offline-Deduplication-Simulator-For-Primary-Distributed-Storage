
typedef struct{
	FILE* inputFile;	//Contains a list of memory addresses that the VM wants to access
	int id;
}VM;		// Corresponds to a VM running on a server

typedef struct{
	vector<long long>* unreferencedQueuePtr;
	vector<long long>* dirtyQueuePtr;
	vector<long long>* freeQueuePtr;
	unordered_map<pair<int, long long>, long long>* logicalToPhysicalMappingPtr;    // maps (VM id, logical address) : physical address 
	unordered_set<pair<int, long long>>* setOfCOWAddressesPtr;		// Stores the set of logical addresses that are CoW
	// pair<long long, long long>* allocatedExtents;				// Stores list of allocated extents (starting address, length)
	// unordered_map<long long, bool*> allocatedStorage;	//Stores which storage blocks in each extent have been allocated 
}Dom0;		// Refers to the master VM which assists the Xen hypervisor

typedef struct{
	unordered_map<int,VM*>* VMOnServerPtr;		//maps vm id : vm
	Dom0* dom0;
}server;		// Refers to a single server object which can run multiple VMs 

//Creates a new server object with given id. Creates a Dom0 object for the server as well
server* createNewServer(int id);

//Creates a new VM with given file path as the input file and returns it
bool createNewVM(server* serv, int id, string path);

// Scheduler function - returns which VM to process addresses from next and number of addresses to be processed
pair<VM*, int> scheduleNextVM();

//Processes a storage access request. Returns true on successful completion of request
bool processNextAddress(VM* vm);

//Translates a logical address of a particular VM to physical address
long long translateLogicalToPhysical(int vmID, long long logicalAddress);

// Returns true if the logical address is marked COW
bool getLogicalAddressCOW(int vmID, long long logicalAddress);

// Sets the logical address as COW(if COW is true)/ not COW
bool setLogicalAddressCOW(int vmID, long long logicalAddress, bool COW);

// Reads from the given logical address and returns true on completion 
bool read(long long logicalAddress);

// Writes to the given logical address and returns true on completion. Also inserts the block into dirty queue/ unreferenced queue as required
bool write(long long logicalAddress);

// Runs the duplicate finder to aliase potential duplicate blocks. Updates logical-to-physical mapping and the different queues and DDI accordingly
bool runDuplicateFinder();

// Runs duplicate finder on the the next block in the dirty queue.
bool runDuplicateFinderOnNextBlock();

// Runs the garbage collector to free unreferenced blocks. Updates the queues and DDI accordingly
bool runGarbageCollection();

// Runs duplicate finder on the the next block in the unreferenced queue.
bool runGarbageCollectionOnNextBlock();

// VM requests storage space from Dom0. If Dom0 cannot grant it, it requests the extent server
long long* requestStorageAllocationFromDom0(int requestSize); 

//Calculates the hash value of a read block
long long calculateSignatureOfBlock(long long physcialAddress);

bool deleteVM(server* serv,int id);

bool deleteDom0(server* serv);

bool deleteServer(server* serv);









