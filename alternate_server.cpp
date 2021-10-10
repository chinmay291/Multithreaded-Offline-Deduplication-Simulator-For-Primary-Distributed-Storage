#include<bits/stdc++.h>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

typedef struct{
	FILE* inputFile;	//Contains a list of memory addresses that the VM wants to access
	int id;
}VM;		// Corresponds to a VM running on a server

typedef struct{
	vector<long long> unreferencedQueuePtr;
	vector<long long> dirtyQueuePtr;
	vector<long long> freeQueuePtr;
	// unordered_map<pair<int, long long>, long long> logicalToPhysicalMappingPtr;    // maps (VM id, logical address) : physical address 
	// unordered_set<pair<int, long long>> setOfCOWAddressesPtr;		// Stores the set of logical addresses that are CoW
	// pair<long long, long long>* allocatedExtents;				// Stores list of allocated extents (starting address, length)
	// unordered_map<long long, bool*> allocatedStorage;	//Stores which storage blocks in each extent have been allocated 
}Dom0;		// Refers to the master VM which assists the Xen hypervisor

typedef struct{
	unordered_map<int,VM> VMOnServerPtr;		//maps vm id : vm
	Dom0 dom0;
}server;		// Refers to a single server object which can run multiple VMs


bool insertInDirtyQueue(server& s_ptr, long long address){
	s_ptr.dom0.dirtyQueuePtr.push_back(address);
	return true;
} 

int main(){
	printf("sizeof(long long) = %lld\n",sizeof(long long));
	server s; 
	server& s_ptr = s;
	insertInDirtyQueue(s_ptr, 2345);
	printf("%d\n",s_ptr.dom0.dirtyQueuePtr[0]);
	return 0;
}
