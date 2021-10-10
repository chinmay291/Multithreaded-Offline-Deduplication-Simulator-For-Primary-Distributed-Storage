
typedef struct{
	server* serversInDatacenter;
	extentServer* extServer;
}dataCenter;

// Creates and returns a new dataCenter object
dataCenter* createNewDatacenter();

bool deleteDatacenter(dataCenter* dCenter);