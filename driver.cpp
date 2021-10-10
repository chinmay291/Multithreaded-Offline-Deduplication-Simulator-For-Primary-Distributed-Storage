#include<bits/stdc++.h>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include <boost/multiprecision/cpp_int.hpp>
#include "server.h"
#include "DDI.h"
#include "storage.h"

/* Global Variables */
DDI ddi;
extentServer exServer(100);
long long writeThreshold;
unordered_map<int, vector<long long>>COWSignalVectorMap;
sem_t COWSignalVectorMapMutex;

/*
1. Initialize server
2. Initialize VMs and open their input files
3. While some VM still has addresses to process
4. Schedule next VM and decide number of addresses it will proccess at most

(Assuming that input for each type will be of type: logicalAddress R/W)

5. For a read, simply convert logical to physical and read the block contents(ie. add the time)
6. For a write, call the write() function
7. After every n writes(n defined by user in config file), invoke the dupFinder() followed by garbageCollector()
8. When a VM finishes processing all addresses from its input file, close the file
9. When files of all VMs get closed, shut down the server thread
*/

typedef struct{
	vector<string>* VMFilePaths;
	int id;
}serverThreadInput;


void* server_driver(void* input){
	printf("Server Driver: starting\n");
	serverThreadInput* inp = (serverThreadInput*)input;
	vector<string>* VMFilePathsRef = (vector<string>*) inp->VMFilePaths;
	vector<string> VMFilePaths = *VMFilePathsRef;
	int id = inp->id;
	Server serv(id,VMFilePaths.size());
	Server& serverPtr = serv;
	printf("Server Driver: Server created. Server ID = %d\n",serv.id);
	for(int i = 0; i < VMFilePaths.size(); i++){
		char inp_path[VMFilePaths[i].size()+1];
		strcpy(inp_path,VMFilePaths[i].c_str());
		printf("Server driver: Input file path = %s\n",inp_path);
		serv.initializeVM(i+1,inp_path);
	}
	printf("Server Driver: All VMs created for server with ID %d\n", serv.id);
	serv.startServerExecution();
	pthread_exit(0);
}


int main(){
	printf("Driver: Starting simulator\n");
	sem_init(&COWSignalVectorMapMutex,0,1);
	int numServers;
	FILE* configFileFp = fopen("./inputs/input.txt","r");
	long long storageSize;
	fscanf(configFileFp,"%lld",&storageSize);
	fgetc(configFileFp);
	printf("Driver: Storage size(Number of blocks) = %lld\n",storageSize);
	extentServer tempExServer(storageSize);
	exServer = tempExServer;
	fscanf(configFileFp,"%lld",&writeThreshold);
	fgetc(configFileFp);
	printf("Driver: Write threshold = %lld\n",writeThreshold);
	fscanf(configFileFp,"%d",&numServers);
	fgetc(configFileFp);
	printf("Driver: Num servers = %d\n", numServers);
	ddi.initializeSecondLevelIndex(numServers);
	ddi.initializeShards(numServers);
	ddi.initializeDDISemaphores(numServers);
	int i;
	vector<pthread_t> threads;
	vector<vector<string>*> filePaths;
	for(i = 0; i < numServers; i++){
		int numVMs;
		fscanf(configFileFp,"%d",&numVMs);
		fgetc(configFileFp);
		printf("Driver: Number of VMs for server %d: %d\n",i+1,numVMs);
		vector<string>* VMFilePaths = new vector<string>();
		for(int j = 0; j < numVMs; j++){
			char path[100];
			fgets(path,100,configFileFp);
			string pathStr = path;
			pathStr.pop_back();
			// cout << "Path = " << path;
			VMFilePaths->push_back(pathStr);
		}
		filePaths.push_back(VMFilePaths);
		serverThreadInput* inp = (serverThreadInput*)calloc(1, sizeof(serverThreadInput));
		inp->VMFilePaths = VMFilePaths;
		inp->id = i+1;
		pthread_t tid_server;
		pthread_create(&tid_server,NULL,server_driver,(void*)inp);
		printf("Driver: New thread created for server\n");
		threads.push_back(tid_server);
	}
	fclose(configFileFp);
	for(i = 0; i < numServers; i++){
		pthread_join(threads[i], NULL);
	}
	ddi.destoryDDISemaphores(numServers);
	printf("Driver: All server threads joined\n");
	// while(!filePaths.empty()){
	// 	free(filePaths.back());
	// 	filePaths.pop_back();
	// }
	return 0;
}

