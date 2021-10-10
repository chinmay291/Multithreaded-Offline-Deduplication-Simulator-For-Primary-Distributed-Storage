#include<bits/stdc++.h>
/*
A distributed coordination mechanism allocates free blocks from the
storage backend (physical disks) when a logical volume is created, lazily when a block
is written for the first time, or when an aliased block is updated (i.e., copied on write).

Storage extents are allocated with a large granularity and are then, within each physi-
cal host, used to satisfy individual block allocation requests, thus reducing the overhead
of contacting a remote service
*/


typedef struct{
	/* maintains allocation of physical address chunks to servers
		and can also allocate/deallocate chunks
	*/
	int allocationMap*;		//Stores the id of the server that each block has been assigned to. Unallocated blocks have id -1
}extentServer;


// Creates a new extent server object and returns it
extentServer* createNewExtentServer(long long storageSize);

// Returns an array of pairs (starting address, length) 
pair<long long, long long>* allocateExtent(long long requestSize);

// Deallocates a contiguous extent and returns true on success
bool deallocateExtent(long long startAddress, long long extentLength);

// Deallocates a single block 
bool deallocateBlock(long long physicalAddress); 