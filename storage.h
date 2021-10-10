class extentServer{
	/* maintains allocation of physical address chunks to servers
		and can also allocate/deallocate chunks
	*/
	public:
		vector<int> allocationMap;		//Stores the id of the server that each block has been assigned to. Unallocated blocks have id -1
		vector<string>contents;		// Stores the signatues of all blocks. They are updated at every write operation only
		sem_t extentServerMutex;		// Only to be used when requesting for allocation of blocks
		long long nextFitStart;	

		extentServer(long long storageSize){
			// printf("Initializing extent server\n");
			int i;
			for(i = 0; i < storageSize; i++){
				allocationMap.push_back(-1);
				contents.push_back("peeppeep");
			}
			// printf("Initializing mutex of extentServer\n");
			sem_init(&extentServerMutex,0,1);
			nextFitStart = storageSize;
			printf("Extent server initialized\n");
		}
		

		// Returns an array of pairs (starting address, length) 
		vector<pair<long long, long long>> allocateExtent(long long requestSize);

		// Deallocates a single block 
		bool deallocateBlock(long long physicalAddress); 

		// Returns signature of the block with given physical address
		string getSignature(long long physicalAddress);

		// Updates the signature of the given block
		bool updateSignature(long long physicalAddress, string blockSignature);

		// Acquires lock on the extent server for block allocation using a semaphore
		bool lockExtentServer();

		// Releases the acquired semaphore lock on the extent server
		bool unlockExtentServer(); 
};