	//Stores (signature, physical address, no. of references)

	/*
		Doubt: How to decide how to allocate the shards between the servers?
			   What after a server receives a part of another shard from a different server?

	*/


typedef struct {
	unordered_map<int, unordered_map<long long, pair<long long, long long>>>indexShards;	//Stores index shard of each server(server ID, (signature, (physical address, number of references))
	unordered_map<int, sem_t*> shardLocks; // Semaphores for each of the index shards(server ID, semaphore for that server)
}DDI;

bool testAndIncrementDDI(long long signature);	// Checks for a signature within the DDI and increments the number of references. If no block with that signature exists, then adds a new entry to DDI

bool testAndDecrementDDI(long long signature);	// Checks for a signature within the DDI and decrements the number of references. If no references exits to a block with that signature, then removes the entry of the block from DDI

bool deleteDDI(DDI* ddi);