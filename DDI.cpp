/*
		TO DO: Implement setCOWStatusViaDDI() - Are signals required? They are not recommended for thread communication
				Alternate solution: Maintain a global unordered_map with key = server ID and value = a vector storing logical addresses to mark as CoW
				Before every write the thread will check whether any entry has been added to its vector

				In a real physical system, this will most likely be implemented by sending a signal, but since this is a simulator
				we are substituting this signal with this global COWSignalVectorMap approach

*/
#include<bits/stdc++.h>
#include<stdlib.h>
#include<semaphore.h>
#include <boost/multiprecision/cpp_int.hpp>
using namespace std;

#include "DDI.h"


extern unordered_map<int, vector<long long>>COWSignalVectorMap;
extern sem_t COWSignalVectorMapMutex;



bool DDI::initializeSecondLevelIndex(int numServers){
	/* Assumption: The minimum value of the hash is 0 and maximum value is LLONG_MAX */
	namespace mp = boost::multiprecision;
	mp::uint128_t maxMD5 = numeric_limits<mp::uint128_t>::max();
	mp::uint128_t divisionSize = maxMD5/numServers;
	for(int i = 0; i < numServers; i++){
		if(i == 0){
			mp::uint128_t zero = 0;
			pair<mp::uint128_t, mp::uint128_t> p = make_pair(zero, divisionSize);
			secondLevelIndex.push_back(p);
		}
		else{
			pair<mp::uint128_t, mp::uint128_t> p = make_pair(i*divisionSize+1,(i+1)*divisionSize);
			secondLevelIndex.push_back(p);
		}
	}
	return true;
}

bool DDI::initializeShards(int numServers){
	for(int i = 0; i < numServers; i++){
		unordered_map<string, pair<long long, vector<pair<int, long long>>>> m;
		indexShards.push_back(m);	
	}
}

bool DDI::initializeDDISemaphores(int numShards){
	for(int i = 0;i < numShards; i++){
		sem_t* mutex;
		sem_init(mutex,0,1);
		shardLocks.push_back(mutex);
	}
	return true;
}

bool DDI::destoryDDISemaphores(int numShards){
	for(int i = 0; i < numShards; i++){
		sem_destroy(shardLocks[i]);
	}
	return true;
}


int DDI::getShardNumberFromSecondLevelIndex(string blockSignature){
	int shardNumber = -1;
	namespace mp = boost::multiprecision;
	mp::uint128_t maxMD5 = numeric_limits<mp::uint128_t>::max();
	mp::uint128_t signature(blockSignature);
	if(signature == maxMD5){
		// printf("LLONG_MAX SIGNATURE\n");
		return secondLevelIndex.size();
	}
	for(int i = 0; i < secondLevelIndex.size(); i++){
		if(secondLevelIndex[i].first <= signature && secondLevelIndex[i].second > signature){
			shardNumber = i+1;
			break;
		}
	}
	return shardNumber;
}

bool DDI::lockShard(int shardNumber){
	sem_wait(shardLocks[shardNumber-1]);		// Always shardNumber -1: a source of potential bugs
	return true;
}

bool DDI::unlockShard(int shardNumber){
	sem_post(shardLocks[shardNumber-1]);
	return true;
}

long long DDI::queryDDI(string blockSignature, int shardNumber){
	long long matchedAddress;
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		matchedAddress = indexShards[shardNumber-1][blockSignature].first;
	}
	else{
		matchedAddress = -1;
	}
	return matchedAddress;
}

bool DDI::incrementDDI(string blockSignature, long long logicalAddress, int serverID, int shardNumber, FILE* outputFile){
	// For debugging
	// printf("DDI: Gonna fflush\n");
	// fflush(stdout);
	//
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		indexShards[shardNumber-1][blockSignature].second.push_back(make_pair(serverID,logicalAddress));
		// fprintf(outputFile,"DDI: Number of references to the block with signature %s are now = %lld\n", blockSignature,indexShards[shardNumber-1][blockSignature].second.size());
		printf("DDI: Number of references to the block with signature %s are now = %lld\n", blockSignature,indexShards[shardNumber-1][blockSignature].second.size());
		return true;
	}
	else{
		return false;
	}
}

bool DDI::decrementDDI(string blockSignature, long long logicalAddress, int serverID, int shardNumber, FILE* outputFile){
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		auto it = find(indexShards[shardNumber-1][blockSignature].second.begin(),
					    indexShards[shardNumber-1][blockSignature].second.end(), 
						make_pair(serverID,logicalAddress));
		if(it != indexShards[shardNumber-1][blockSignature].second.end()){
			indexShards[shardNumber-1][blockSignature].second.erase(it);
			// fprintf(outputFile,"DDI: Number of references to the block with signature %s are now = %lld\n", blockSignature,indexShards[shardNumber-1][blockSignature].second.size());
			printf("DDI: Number of references to the block with signature %s are now = %lld\n", blockSignature,indexShards[shardNumber-1][blockSignature].second.size());
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

bool DDI::insertNewEntryInDDI(string blockSignature, long long physicalAddress, long long logicalAddress, int serverID, int shardNumber, FILE* outputFile){
	if(indexShards[shardNumber-1].find(blockSignature) == indexShards[shardNumber-1].end()){
		vector<pair<int, long long>> v;
		v.push_back(make_pair(serverID,logicalAddress));
		indexShards[shardNumber-1][blockSignature] = make_pair(physicalAddress,v);
		// fprintf(outputFile,"DDI: Entry for signature %s inserted\n",blockSignature);
		printf("DDI: Entry for signature %s inserted\n",blockSignature);
		return true;
	}
	else{
		return false;
	}
}

bool DDI::setCOWStatusViaDDI(string blockSignature, int shardNumber){
	sem_wait(&COWSignalVectorMapMutex);;
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		vector<pair<int, long long>> v = indexShards[shardNumber-1][blockSignature].second; 
		for(int i = 0; i < v.size(); i++){
			COWSignalVectorMap[v[i].first].push_back(v[i].second);
		}
		sem_post(&COWSignalVectorMapMutex);
		return true;
	}
	else{
		sem_post(&COWSignalVectorMapMutex);
		return false;
	}

}

bool DDI::removeCOWStatusViaDDI(string blockSignature, int shardNumber){
	sem_wait(&COWSignalVectorMapMutex);
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		vector<pair<int, long long>> v = indexShards[shardNumber-1][blockSignature].second; 
		for(int i = 0; i < v.size(); i++){
			auto it = find(COWSignalVectorMap[v[i].first].begin(), COWSignalVectorMap[v[i].first].end(), v[i].second);
			if(it != COWSignalVectorMap[v[i].first].end()){
				COWSignalVectorMap[v[i].first].erase(it);
			}
		}
		sem_post(&COWSignalVectorMapMutex);
		return true;
	}
	else{
		sem_post(&COWSignalVectorMapMutex);
		return false;
	}
}

int DDI::getNumberOfReferencesFromDDI(string blockSignature, int shardNumber){
	int numReferences = 0;
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		numReferences = indexShards[shardNumber-1][blockSignature].second.size();
	}
	return numReferences;
}

bool DDI::deleteEntryFromDDI(string blockSignature, int shardNumber, FILE* outputFile){
	if(indexShards[shardNumber-1].find(blockSignature) != indexShards[shardNumber-1].end()){
		indexShards[shardNumber-1].erase(blockSignature);
		// fprintf(outputFile,"DDI: Entry for signature %s removed\n",blockSignature);
		printf("DDI: Entry for signature %s removed\n",blockSignature);
		return true;
	}
	else{
		return false;
	}
}












