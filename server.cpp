
/*
TO DO: 
1. Implement reading input VM traces
2. Implement timing functions(which count time req for every activity
3. If after the write the COW block only has one reference then set it as non-COW
*/

//Processes a storage access request for the given VM id. Returns true on successful completion of request
// bool processNextAddress(int VMid);

#include<stdio.h>
#include<stdlib.h>
#include<semaphore.h>
#include<bits/stdc++.h>
using namespace std;
#include <boost/multiprecision/cpp_int.hpp>
#include "server.h"
#include "DDI.h"
#include "storage.h"


extern DDI ddi;
extern extentServer exServer;
extern unordered_map<int, vector<long long>>COWSignalVectorMap;
extern int writeThreshold;
extern sem_t COWSignalVectorMapMutex;


bool Server::startServerExecution(){
	int terminateServer = VMOnServer.size();
	// fprintf(outputFile,"terminateServer = %d\n",terminateServer);
	int numWrites = 0;
	while(terminateServer > 0){
		int vmID = scheduleNextVM();
		// fprintf(outputFile,"-----------------------------------------------------------------------------------------------------------\n");
		// fprintf(outputFile,"ID of next scheduled VM is : %d\n",vmID);
		printf("-----------------------------------------------------------------------------------------------------------\n");
		printf("ID of next scheduled VM is : %d\n",vmID);
		int numAddressesToProcess = 10; // Random value between 1 and _______(ask sir)
		for(int i = 0; i < numAddressesToProcess; i++){
			long long nextAddress = readNextAddressFromInputFile(vmID);	
			// fprintf(outputFile,"Address read from input file is: %lld\n",nextAddress);		
			printf("Server driver %d: Address read from input file is: %lld\n", id,nextAddress);		
			if(nextAddress == -1){
				terminateServer--;
				break;
			}
			char operationType = readNextOperationTypeFromInputFile(vmID);
			// fprintf(outputFile,"Operation type is: %c\n",operationType);	
			printf("Server driver %d: Operation type is: %c\n", id, operationType);	
			if(operationType == 'R'){
				processRead(nextAddress);	
				// fprintf(outputFile,"Read complete\n");		
				printf("Read complete\n");		
			}
			else if(operationType == 'W'){
				string signature = readNextSignatureFromInputFile(vmID);
				// fprintf(outputFile,"Signature of the block after writing is: %lld\n",signature);
				write(nextAddress, signature);
				cout << "Server driver " << id << ": Signature of the block after writing is: " << signature << endl;
				numWrites++;
				// fprintf(outputFile,"Write complete. numWrites = %d\n", numWrites);
				// fprintf(outputFile,"-----------------------------------------------------------------------------------------------------------\n");
				printf("Server driver %d: Write complete. numWrites = %d\n", id,numWrites);
				printf("-----------------------------------------------------------------------------------------------------------\n");
				if(numWrites == writeThreshold){
					break;
				}
			}			
			else{
				// TO DO: Log invalid operation
			}
		}
		if(numWrites == writeThreshold){
			int numAddressesForDupFinder = numWrites / 2 + 1;			// Arbitrarily set 	
			int numAddressesForGC = numAddressesForDupFinder / 2;		// Arbitrarily set
			for(int i = 0; i < numAddressesForDupFinder; i++){
				if(dom0.dirtyQueue.size() > 0){
					runDuplicateFinderOnNextBlock();	
				}
			}
			for(int i = 0; i < numAddressesForDupFinder; i++){
				if(dom0.unreferencedQueue.size() > 0){
					runGarbageCollectionOnNextBlock();	
				}
				else{
					break;
				}
			}
			numWrites = 0;	
		}
	}
	while(dom0.dirtyQueue.size() > 0){
		runDuplicateFinderOnNextBlock();	
	}
	while(dom0.unreferencedQueue.size() > 0){
		runGarbageCollectionOnNextBlock();	
	}
	// fprintf(outputFile, "Server Driver: Server execution complete\n");
	printf("Server Driver %d: Server execution complete\n", id);
	fflush(outputFile);
	fclose(outputFile);
	return true; 
}

bool Server::initializeVM(int id, char* inputFilePath){
	// fprintf(outputFile,"Server driver: Initializing VM\n");
	printf("Server driver: Initializing VM\n");
	VM vm;
	vm.id = id;
	// fprintf(outputFile,"Server driver: VM id assigned\n");
	printf("Server driver: VM id assigned\n");
	vm.inputFile = fopen(inputFilePath,"r");
	if(vm.inputFile == NULL){
		printf("NULL FILE POINTER\n");
	}
	// fprintf(outputFile,"Server driver: VM input file opened\n");
	printf("Server driver: VM input file opened\n");
	VMOnServer[vm.id-1] = vm;
	// fprintf(outputFile,"Server driver: VM initialzation complete\n");
	printf("Server driver: VM initialzation complete\n");
	return true;
}


bool Server:: write(long long logicalAddress, string signature){
	/*
		1. Convert logical address to physical address. 
		2. If no physical address has been allocated to logical address, get a block from free queue and add an entry to logical to physical map
		2. Check if logical address is COW
		3. If address is not COW, update it in-place
		4. If address is COW, get a new block from freeQueue and write to it. Place old block in unreferenced queue
		5. If COW was performed, update the logical to physical mapping
		6. Place the block that has been written to in the dirty queue.

		
	*/
	bool COW = checkCOWStatus(logicalAddress);
	// fprintf(outputFile,"COW status of %lld is %d\n",logicalAddress, COW);
	printf("Server driver %d: COW status of %lld is %d\n", id, logicalAddress, COW);
	long long physicalAddress = convertLogicalToPhysical(logicalAddress);
	// fprintf(outputFile,"Physical address corresponding to %lld is %lld\n",logicalAddress, physicalAddress);
	printf("Server driver %d: Physical address corresponding to %lld is %lld\n", id, logicalAddress, physicalAddress);
	if(physicalAddress == -1){
		long long freeBlockAddress = getBlockFromFreeQueue();
		// fprintf(outputFile,"Address of free block gotten from free queue is %lld\n",freeBlockAddress);	
		printf("Server driver %d: Address of free block gotten from free queue is %lld\n", id, freeBlockAddress);	
		//Update signature value in storage
		exServer.updateSignature(freeBlockAddress, signature);
		updateLogicalToPhysicalMapping(logicalAddress, freeBlockAddress);		//Insertion step
		insertIntoDirtyQueue(logicalAddress);
		return true;
	}
	if(COW){
		//Update signature value in storage
		long long freeBlockAddress = getBlockFromFreeQueue();
		// fprintf(outputFile,"Address of free block gotten from free queue is %lld\n",freeBlockAddress);	
		printf("Server driver %d: Address of free block gotten from free queue is %lld\n", id, freeBlockAddress);	
		exServer.updateSignature(freeBlockAddress, signature);
		insertIntoUnreferencedQueue(logicalAddress, physicalAddress);
		updateLogicalToPhysicalMapping(logicalAddress, freeBlockAddress);
		setCOWStatus(logicalAddress, false);
	}
	else{
		//Update signature value in storage
		exServer.updateSignature(physicalAddress, signature);	
	}
	insertIntoDirtyQueue(logicalAddress);
	return true;
}

bool Server::runDuplicateFinderOnNextBlock(){
	/*
		1. Get next block address from dirty queue(let's call it A)
		3. Read the block and calculate its signature
		4. Lock the shard of the DDI which may index that signature 
		5. Query the DDI with the signature: DDI will return address of duplicate block(let's call it B) 
		6. Mark B as COW 
		7. Read B and calculate its signature
		8. If signature of A == signature of B then update logical to physical mapping from A to B
		9. Increment the number of references to B in the DDI
		10. Put A in free queue
		11. If there is no potential alias, then insert new entry into DDI
		12. Unlock DDI shard
		
	*/
	long long dirtyBlockLogicalAddress = getBlockFromDirtyQueue(); 
	// fprintf(outputFile,"Duplicate finder: Logical address of the dirty block is: %lld\n", dirtyBlockLogicalAddress);
	printf("Duplicate finder %d: Logical address of the dirty block is: %lld\n", id, dirtyBlockLogicalAddress);
	long long dirtyBlockPhysicalAddress = convertLogicalToPhysical(dirtyBlockLogicalAddress);
	// fprintf(outputFile,"Duplicate finder: Physical address of the dirty block is: %lld\n", dirtyBlockPhysicalAddress);
	printf("Duplicate finder %d: Physical address of the dirty block is: %lld\n", id, dirtyBlockPhysicalAddress);
	string dirtyBlockSignature = exServer.getSignature(dirtyBlockPhysicalAddress);
	// fprintf(outputFile,"Duplicate finder: Signature of the dirty block is: %s\n", dirtyBlockSignature);
	cout << "Duplicate finder " << id << ": Signature of the dirty block is: " << dirtyBlockSignature << endl;
	int shardNumber = ddi.getShardNumberFromSecondLevelIndex(dirtyBlockSignature);
	// fprintf(outputFile,"Duplicate finder: Shard number for the signature is: %d\n",shardNumber);
	printf("Duplicate finder %d: Shard number for the signature is: %d\n", id, shardNumber);
	ddi.lockShard(shardNumber);
	// fprintf(outputFile,"Shard locked. Querying DDI now\n");
	printf("Duplicate finder %d: Shard locked. Querying DDI now\n", id);
	long long potentialAliasAddress = ddi.queryDDI(dirtyBlockSignature, shardNumber);
	if(potentialAliasAddress != dirtyBlockPhysicalAddress && potentialAliasAddress != -1){
		// fprintf(outputFile,"Duplicate Finder: Found a potential alias with address %lld in the DDI.\n", potentialAliasAddress);
		printf("Duplicate Finder %d: Found a potential alias with physical address %lld in the DDI.\n", id, potentialAliasAddress);
		ddi.setCOWStatusViaDDI(dirtyBlockSignature, shardNumber);
		printf("Duplicate Finder %d: DEBUG - COW status set via DDI\n", id);
		fflush(stdout);
		string potentialAliasSignature = exServer.getSignature(potentialAliasAddress);
		printf("Duplicate Finder %d: DEBUG - Obtained signature\n", id);
		fflush(stdout);
		if(dirtyBlockSignature == potentialAliasSignature){
			// fprintf(outputFile,"Alias confirmed\n");
			printf("Duplicate Finder %d: Alias confirmed\n", id);
			updateLogicalToPhysicalMapping(dirtyBlockLogicalAddress, potentialAliasAddress);
			printf("Duplicate Finder %d: DEBUG - L2P mapping updated\n", id);
			fflush(stdout);
			setCOWStatus(dirtyBlockLogicalAddress, true);
			printf("Duplicate Finder %d: DEBUG - COW status set\n", id);
			fflush(stdout);
			ddi.incrementDDI(potentialAliasSignature, dirtyBlockLogicalAddress, id, shardNumber, outputFile);
			printf("Duplicate Finder %d: DEBUG - DDI incremented\n", id);
			fflush(stdout);
			insertIntoFreeQueue(dirtyBlockPhysicalAddress);
			printf("Duplicate Finder %d: DEBUG - Inserted into free queue\n", id);
			fflush(stdout);
		}
		else{
			// fprintf(outputFile,"Duplicate finder: It wasn't actually an alias\n");
			printf("Duplicate finder %d: It wasn't actually an alias\n", id);
			ddi.removeCOWStatusViaDDI(dirtyBlockSignature, shardNumber);
			ddi.insertNewEntryInDDI(dirtyBlockSignature, dirtyBlockPhysicalAddress, dirtyBlockLogicalAddress, id, shardNumber, outputFile);
		}	
	}
	else if(potentialAliasAddress != dirtyBlockPhysicalAddress){
		// fprintf(outputFile,"Signature entry is not present in DDI. Adding it now\n");
		printf("Duplicate finder %d: Signature entry is not present in DDI. Adding it now\n", id);
		ddi.insertNewEntryInDDI(dirtyBlockSignature, dirtyBlockPhysicalAddress, dirtyBlockLogicalAddress, id, shardNumber, outputFile);
	}
	ddi.unlockShard(shardNumber);
	// fprintf(outputFile,"Shard unlocked\n");
	// fprintf(outputFile,"-----------------------------------------------------------------------------------------------------------\n");
	printf("Duplicate finder %d: Shard unlocked\n", id);
	printf("-----------------------------------------------------------------------------------------------------------\n");
	return true;
}

bool Server::runGarbageCollectionOnNextBlock(){
	/*
		1. Get next block address from unreferenced queue(let's call it A)
		2. Read A and calculate it's signature
		3. Lock the shard of the DDI which may index that signature 
		4. Query the DDI: It will return address of block B
		5. If A == B, then reduce no. of references to B. If number of references to B is zero then remove the entry from the DDI and put B in the free queue
		6. If A != B, then put A in the free queue   
	*/
	// long long potentialUnreferencedBlockPhysicalAddress = getBlockFromUnreferencedQueue();
	pair<long long, long long> p = getBlockFromUnreferencedQueue();
	long long potentialUnreferencedBlockLogicalAddress = p.first;
	long long potentialUnreferencedBlockPreviousPhysicalAddress = p.second;
	string potentialUnreferencedBlockSignature = exServer.getSignature(potentialUnreferencedBlockPreviousPhysicalAddress);
	// fprintf(outputFile,"Garbage collector: Running on block with physical address %lld and signature %lld\n",potentialUnreferencedBlockPreviousPhysicalAddress, potentialUnreferencedBlockSignature);	
	cout << "Garbage collector " << id << ": Running on block with physical address "<< potentialUnreferencedBlockPreviousPhysicalAddress << "and signature " << potentialUnreferencedBlockSignature << endl;
	int shardNumber = ddi.getShardNumberFromSecondLevelIndex(potentialUnreferencedBlockSignature);
	ddi.lockShard(shardNumber);
	// fprintf(outputFile,"Shard locked. Querying DDI now\n");
	printf("Garbage collector %d: Shard locked. Querying DDI now\n", id);
	long long comparisonAddress = ddi.queryDDI(potentialUnreferencedBlockSignature, shardNumber);
	if(potentialUnreferencedBlockPreviousPhysicalAddress == comparisonAddress){
		int numReferences = ddi.getNumberOfReferencesFromDDI(potentialUnreferencedBlockSignature, shardNumber);
		if(numReferences == 1){
			// fprintf(outputFile,"Garbage collector: Block is unreferenced. Deleting entry from DDI\n");
			printf("Garbage collector %d: Block is unreferenced. Deleting entry from DDI\n", id);
			ddi.deleteEntryFromDDI(potentialUnreferencedBlockSignature, shardNumber, outputFile);
			insertIntoFreeQueue(potentialUnreferencedBlockPreviousPhysicalAddress);
		}
		else{
			// fprintf(outputFile,"Garbage collector: Decrementing number of references to the block\n");
			printf("Garbage collector %d: Decrementing number of references to the block\n", id);
			ddi.decrementDDI(potentialUnreferencedBlockSignature, potentialUnreferencedBlockLogicalAddress,id, shardNumber, outputFile);
		}
	}
	else{
		insertIntoFreeQueue(potentialUnreferencedBlockPreviousPhysicalAddress);
	}
	ddi.unlockShard(shardNumber);
	// fprintf(outputFile,"Shard unlocked\n");
	// fprintf(outputFile,"-----------------------------------------------------------------------------------------------------------\n");
	printf("Garbage collector %d: Shard unlocked\n", id);
	printf("-----------------------------------------------------------------------------------------------------------\n");
	return true;
} 

bool Server::checkCOWStatus(long long logicalAddress){
	if(dom0.setOfCOWAddresses.find(logicalAddress) != dom0.setOfCOWAddresses.end())
		return true;
	else{
		bool status;
		sem_wait(&COWSignalVectorMapMutex);
		if(find(COWSignalVectorMap[id].begin(), COWSignalVectorMap[id].end(), logicalAddress) != COWSignalVectorMap[id].end())
			status = true;
		else
			status = false;
		sem_post(&COWSignalVectorMapMutex);
		return status; 	
	} 
	
}

bool Server::setCOWStatus(long long logicalAddress, bool COWvalue){
	if(COWvalue == true){
		if(dom0.setOfCOWAddresses.find(logicalAddress) != dom0.setOfCOWAddresses.end())
			return true;
		else{
			sem_wait(&COWSignalVectorMapMutex);
			if(find(COWSignalVectorMap[id].begin(), COWSignalVectorMap[id].end(), logicalAddress) != COWSignalVectorMap[id].end())
				sem_post(&COWSignalVectorMapMutex);
			else{
				dom0.setOfCOWAddresses.insert(logicalAddress);
				sem_post(&COWSignalVectorMapMutex);
			}
			return true;
		}		
	}
	else{
		if(dom0.setOfCOWAddresses.find(logicalAddress) != dom0.setOfCOWAddresses.end()){
			dom0.setOfCOWAddresses.erase(logicalAddress);
			return true;
		}
		else{
			sem_wait(&COWSignalVectorMapMutex);
			if(find(COWSignalVectorMap[id].begin(), COWSignalVectorMap[id].end(), logicalAddress) != COWSignalVectorMap[id].end()){
				auto it = find(COWSignalVectorMap[id].begin(), COWSignalVectorMap[id].end(), logicalAddress);
				COWSignalVectorMap[id].erase(it);
				sem_post(&COWSignalVectorMapMutex);
			}
			return true;
		} 
	}
}

long long Server::convertLogicalToPhysical(long long logicalAddress){
	if(dom0.logicalToPhysicalMapping.find(logicalAddress) != dom0.logicalToPhysicalMapping.end()){
		return dom0.logicalToPhysicalMapping[logicalAddress];
	}
	else{
		return -1;
	}
}

bool Server::updateLogicalToPhysicalMapping(long long logicalAddress, long long freeBlockAddress){
	dom0.logicalToPhysicalMapping[logicalAddress] = freeBlockAddress;
	return true;
}

long long Server::getBlockFromFreeQueue(){
	long long freeBlockAddress;
	if(dom0.freeQueue.size() != 0){
		freeBlockAddress = dom0.freeQueue.front();
		dom0.freeQueue.pop();
	}
	else{
		// TO DO: Check whether allocated blocks is not empty(error condition)
		// fprintf(outputFile,"Free queue is empty, requesting allocation from extent server\n");
		printf("Server driver %d: Free queue is empty, requesting allocation from extent server\n", id);
		vector<pair<long long, long long>>allocatedBlocks =  exServer.allocateExtent(1000); 			// random request size for now
		for(int i = 0; i < allocatedBlocks.size(); i++){
			pair<long long, long long>p = allocatedBlocks[i];
			printf("Server driver %d: start = %lld, end = %lld\n", id, p.first, p.second);
			for(long long j = p.first; j<= p.second; j++){
				dom0.freeQueue.push(j);
			}
		}
		freeBlockAddress = dom0.freeQueue.front();
		// fprintf(outputFile,"Size of free queue after allocation from extent server = %ld\n", dom0.freeQueue.size());
		dom0.freeQueue.pop();
	}
	return freeBlockAddress;
}

long long Server::getBlockFromDirtyQueue(){
	long long returnAddress;
	if(dom0.dirtyQueue.size() != 0){
		returnAddress = dom0.dirtyQueue.front();
		dom0.dirtyQueue.pop();
	}
	else{
		returnAddress = -1;
	}
	return returnAddress;
}

pair<long long, long long> Server::getBlockFromUnreferencedQueue(){
	pair<long long, long long> returnAddress;
	if(dom0.unreferencedQueue.size() != 0){
		returnAddress = dom0.unreferencedQueue.front();
		dom0.unreferencedQueue.pop();
	}
	else{
		returnAddress = make_pair(-1, -1);
	}
	return returnAddress;
}

bool Server::insertIntoUnreferencedQueue(long long logicalAddress, long long physicalAddress){
	dom0.unreferencedQueue.push(make_pair(logicalAddress,physicalAddress));
	return true;
}

bool Server::insertIntoDirtyQueue(long long logicalAddress){
	dom0.dirtyQueue.push(logicalAddress);
	return true;
}

bool Server::insertIntoFreeQueue(long long physicalAddress){
	dom0.freeQueue.push(physicalAddress);
	return true;
}

int Server::scheduleNextVM(){
	currVMRunningID = currVMRunningID + 1;
	if(currVMRunningID > VMOnServer.size()){
		currVMRunningID = 1;
	}
	return currVMRunningID;
}

long long Server::readNextAddressFromInputFile(int vmID){
	long long address;
	fscanf(VMOnServer[vmID-1].inputFile, " ");
	bool eof = fscanf(VMOnServer[vmID-1].inputFile, "%lld", &address) == EOF;
	if(eof == false){
		// getc(VMOnServer[vmID-1].inputFile);	// Reads the space
		// printf("Address = %lld\n", address);
		return address;
	}
	else{
		printf("EOF is true\n");
		fclose(VMOnServer[vmID-1].inputFile);
		return -1;
	}
}

char Server::readNextOperationTypeFromInputFile(int vmID){
	char operationType;
	fscanf(VMOnServer[vmID-1].inputFile, " ");
	bool eof = fscanf(VMOnServer[vmID-1].inputFile, "%c", &operationType) == EOF;
	if(eof == false){
		// getc(VMOnServer[vmID-1].inputFile);	// Reads the newline char
		return operationType;
	}
	else{
		fclose(VMOnServer[vmID-1].inputFile);
		return 'E';
	}
}

string Server::readNextSignatureFromInputFile(int vmID){
	// printf("Inside readNextSignatureFromInputFile\n");
	char sigArr[200];
	fscanf(VMOnServer[vmID-1].inputFile, " ");
	bool eof = fscanf(VMOnServer[vmID-1].inputFile, "%s", sigArr) == EOF;
	if(eof == false){
		// getc(VMOnServer[vmID-1].inputFile);	// Reads the space
		string signature = sigArr;
		return signature;
	}
	else{
		fclose(VMOnServer[vmID-1].inputFile);
		return "-1";
	}
}

bool Server::processRead(long long logicalAddress){
	// Add timing value here
	return true;
}
















