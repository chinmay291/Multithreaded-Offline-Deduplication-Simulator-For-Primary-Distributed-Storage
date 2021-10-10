// namespace mp = boost::multiprecision;
class DDI{
	public:

	vector<unordered_map<string, pair<long long, vector<pair<int, long long>>>>>indexShards;	//Stores index shard of each server: (signature, (physical address, vector storing (VM id, logical address) of all logical addr that refer to that physical addr)
	vector<sem_t*> shardLocks; // Semaphores for each of the shards 
	vector<pair<boost::multiprecision::uint128_t, boost::multiprecision::uint128_t>> secondLevelIndex; // For each server, it stores the boundaries of the hash space it indexes
	
	// Divides the total hash space among the servers and sets up the second level index accordingly
	bool initializeSecondLevelIndex(int numServers);
	
	// Initializes the semaphores used to lock and unlock the index shards
	bool initializeDDISemaphores(int numShards);	

	// intializes indexShards
	bool initializeShards(int numServers);

	// Destroys the semaphores used to lock and unlock the index shards
	bool destoryDDISemaphores(int numShards);

	// Queries the second level index and obtains the ID of the server which holds the shard that may contain the passed signature
	int getShardNumberFromSecondLevelIndex(string blockSignature);

	// Locks the shard using semaphore
	bool lockShard(int shardNumber);

	// Unlocks the shard that was locked using semaphore
	bool unlockShard(int shardNumber);

	// Checks whether an entry with signature = blockSignature exists in the given shard and returns the physical address of the block with that 
	// signature if the equality is satisfied. Returns -1 otherwise
	long long queryDDI(string blockSignature, int shardNumber);

	// Increments the number of references to the physical block with signature = blockSignature and adds (serverID, logicalAddress) to its list
	// of references
	bool incrementDDI(string blocksignature, long long logicalAddress, int serverID, int shardNumber, FILE* outputFile);

	// Decrements the number of references to the physical block with signature = blockSignature and removes (serverID, logicalAddress) from its list
	// of references
	bool decrementDDI(string blocksignature, long long logicalAddress, int serverID, int shardNumber, FILE* outputFile);

	// Inserts a new entry with key = blocksignature and value = (physicalAddress, (1,[(id,logicalAddress)])) into the DDI
	bool insertNewEntryInDDI(string blockSignature, long long physicalAddress, long long logicalAddress, int id, int shardNumber, FILE* outputFile);	

	// Sets all logical addresses present in the reference list of the key blockSignature as COW
	bool setCOWStatusViaDDI(string blockSignature, int shardNumber);	

	// Sets all logical addresses present in the reference list of the key blockSignature as  not COW
	bool removeCOWStatusViaDDI(string blockSignature, int shardNumber);	

	// Returns number of references to a block with key = blockSignature
	int getNumberOfReferencesFromDDI(string blockSignature, int shardNumber);

	// Deletes an  entry with key = blockSignature from DDI with signature = blockSignature
	bool deleteEntryFromDDI(string blockSignature, int shardNumber, FILE* outputFile);
};