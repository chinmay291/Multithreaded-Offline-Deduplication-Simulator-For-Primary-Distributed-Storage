#include<bits/stdc++.h>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include "server.h"



/*
	Put extern objects here
*/

server* createNewServer(int id){
	server* serv = (server*)calloc(1, sizeof(server));
	Dom0* dom0 = (Dom0*)calloc(1,sizeof(Dom0)); 
	
	vector<long long>* unreferencedQueuePtr = new vector<long long>();
	vector<long long>* freeQueuePtr = new vector<long long>();
	vector<long long>* dirtyQueuePtr = new vector<long long>();
	unordered_map<pair<int, long long>, long long>* logicalToPhysicalMappingPtr = new unordered_map<pair<int, long long>, long long>();
	unordered_set<pair<int, long long>>* setOfCOWAddressesPtr = new unordered_set<pair<int, long long>>(); 

	dom0->unreferencedQueuePtr = unreferencedQueuePtr;
	dom0->freeQueuePtr = freeQueuePtr;
	dom0->dirtyQueuePtr = dirtyQueuePtr;
	dom0->logicalToPhysicalMappingPtr = logicalToPhysicalMappingPtr;
	dom0->setOfCOWAddressesPtr = setOfCOWAddressesPtr;

	serv->dom0 = dom0;
	serv->VMOnServerPtr = new unordered_map<int,VM*>();
	serv->id = id;
	return serv;
}

bool createVM(server* serv, int id, string path){
	if(serv->VMOnServerPtr->find(id) != serv->VMOnServerPtr->end()){
		printf("VM with same ID exists\n");
		return false;
	}
	FILE* VMfp = fopen(path,'r');
	if(VMfp == NULL){
		printf("VM input file failed to open\n");
		return false;
	}
	VM* newVM = (VM*)calloc(1, sizeof(VM));
	newVM->id = id;
	newVM->inputFile = VMfp;
	serv->VMOnServerPtr->insert(make_pair(id,newVM));
	return true;
}

bool deleteVM(server* serv,int id){
	VM* vm = serv->VMOnServerPtr->at(id);
	fclose(vm->inputFile);
	free(vm);
	serv->VMOnServerPtr->erase(id);
	return true;
}

bool deleteDom0(server* serv){
	Dom0* dom0 = serv->dom0;
	delete(dom0->unreferencedQueuePtr);
	delete(dom0->freeQueuePtr);
	delete(dom0->dirtyQueuePtr);
	delete(dom0->logicalToPhysicalMappingPtr);
	delete(dom0->setOfCOWAddressesPtr);
	free(dom0);
	serv->dom0 = NULL;
	return true;
}

bool deleteServer(server* serv){
	deleteDom0(serv);
	for(auto i = serv->VMOnServerPtr->begin(); i != serv->VMOnServerPtr->end(); i++){
		deleteVM(serv, i->first);		
	}
	delete(serv->VMOnServerPtr);
	free(serv);
	return true;
}

int main(){
	server* myServer = createNewServer(1);
	createVM(myServer,1,"./tempInput.txt");
	deleteServer(myServer);
	return 0;
}