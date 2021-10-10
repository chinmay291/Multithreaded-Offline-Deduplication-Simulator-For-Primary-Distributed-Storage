
/*
A distributed coordination mechanism allocates free blocks from the
storage backend (physical disks) when a logical volume is created, lazily when a block
is written for the first time, or when an aliased block is updated (i.e., copied on write).

Storage extents are allocated with a large granularity and are then, within each physi-
cal host, used to satisfy individual block allocation requests, thus reducing the overhead
of contacting a remote service
*/

/*
	TO DO: Implement allocateExtent()
*/

#include<bits/stdc++.h>
#include<semaphore.h>
using namespace std;

#include "storage.h"

vector<pair<long long, long long>> extentServer:: allocateExtent(long long requestSize){
	lockExtentServer();
	printf("Extent server: Starting allocation for request size = %lld\n",requestSize);
	if(nextFitStart == allocationMap.size()){
		nextFitStart = 0;
	}
	long long start = nextFitStart;
	// printf("start at starting  = %lld\n",start);
	long long i,j, end;
	end = 0;
	int flag = 0;
	while(start < allocationMap.size() && end < allocationMap.size() && end - start < requestSize){
		if(start == nextFitStart && flag == 1){
			break;
		}
		if(allocationMap[start] == -1){
			for(j = start+1; j<allocationMap.size(); j++){
				if(j - start <= requestSize){
					// printf("allocationMap[%lld] = %d\n", j,allocationMap[j]);
					if(allocationMap[j] != -1){
						// printf("Here\n");
						start = j+1;
						break;
					}
					else if(j - start == requestSize){
						end = j;
						printf("j = %lld. Breaking\n",end);
						break;
					}
					else{
						end = j;
					}
				}
			}
		}
		else{
			while(start < allocationMap.size() && allocationMap[start] != -1){
				start++;
				if(start == nextFitStart){
					flag = 1;
					break;
				}
			}
		}
		if(end - start == requestSize){
			break;
		}
		if(start == allocationMap.size()){
			flag = 1;
			start = 0;
		}
	}
	// printf("Start = %lld\n",start);
	// printf("End = %lld\n",end);
	nextFitStart = end + 1;
	vector<pair<long long, long long>> allocatedBlocks;
	allocatedBlocks.push_back(make_pair(start, end));
	for(int i = start; i <= end; i++){
		allocationMap[i] = 1;
	}
	unlockExtentServer();
	return allocatedBlocks;
}

bool extentServer::deallocateBlock(long long physicalAddress){
	lockExtentServer();
	allocationMap[physicalAddress] = -1;
	unlockExtentServer();
	return true;
}

string extentServer::getSignature(long long physicalAddress){
	lockExtentServer();
	string signature = contents[physicalAddress];
	unlockExtentServer();
	return signature;
}

bool extentServer::updateSignature(long long physicalAddress, string blockSignature){
	// cout << "Inside updateSignature. Blocsignature = " << blockSignature << endl; 
	lockExtentServer();
	contents[physicalAddress] = blockSignature;
	unlockExtentServer();
	return true;
}

bool extentServer::lockExtentServer(){
	sem_wait(&extentServerMutex);
	return true;
}

bool extentServer::unlockExtentServer(){
	sem_post(&extentServerMutex);
	return true;
}

