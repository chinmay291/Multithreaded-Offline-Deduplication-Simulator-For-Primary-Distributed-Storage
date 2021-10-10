# Multithreaded-Offline-Deduplication-Simulator-For-Primary-Distributed-Storage

## Premise
As the amount of generated data increases, so does the amount of duplicate data that is stored in the same storage cluster. 
Thus, storage cost savings can become quite significant if we are able to eliminate the duplicate data and instead store just one copy of it.
Data deduplication aims to achieve just this. There are two types of data deduplication:
1. Online - Duplicate data detection happens on the write path. As a result, duplicate data is never written and cost savings are maximised.
This occurs at the expense of increased write latency, which can impact an application's performance. 
2. Offline - Duplicate data detection happens in the background after data is written. This approach is preferred because it minimises the impact 
to the application performance. 

In this project, I have implemented a multithreaded offline data deduplication simulator in C++, inspired by the method proposed by Paulo and Pereira in their 2016 ACM
Transactions on Storage paper - https://dl.acm.org/doi/10.1145/2876509

## Description of files
1. storage.h and storage.cpp: Contain the definition and implementation of the extentServer class. The extentServer is responsible for allocating logical blocks of addresses 
to the other servers present in the storage system. It also stores the signature(hash) of each allocated block, which is used for detecting duplicate data.

2. server.h and server.cpp: Contain the definition and implementation of the Server class. A Server contains multiple VMs and a dom0 VM which acts as the master.
dom0 stores 3 queues: freeQueue, dirtyQueue and unreferencedQueue. The blocks allocated by the extentServer are stored by the dom0 in freeQueue. Blocks freed after
garbage collection are also stored in freeQueue. dirtyQueue as the name suggests, stores dirty blocks which have been written recently. The blocks present in this queue
are periodically checked for duplicate data and deduplicated. A copy-on-write mechanism is used for blocks which store deduplicated data and hence have multiple references.
This results in blocks having zero references sometimes, and these are periodically garbage collected
Apart from that, dom0 also does VM scheduling. Each VM has an input file that it reads memory addresses from. These can be obtained from several VM traces available
online such as SNIA IOTTA VM traces - http://iotta.snia.org/traces/block-io/390

3. DDI.h and DDI.cpp: DDI stands for Distributed Duplicates Index. It is a distributed, sharded hash table indexed by the hash value of a block's contents.
During the process of duplicate block checking, the dom0 of
a server queries the DDI by acquiring a write lock for the appropriate shard. If an entry is found, it means that a duplicate block exists. If no entry is found, 
a new entry for the block is added. 

4. dataCenter.h: Contains a wrapper object which stores a list of all Servers and the extentServer present in the datacenter

5. driver.cpp: Initiates Servers and their VMs and orchestrates the whole process of disk I/O operations occuring in the entire datacentre. It's mechanism of action is as follows:
a. Initialize Servers

b. Initialize VMs and open their input files

c. While some VM still has inputs to process

d. Schedule next VM and decide number of addresses it will proccess at most
(Assuming that input for each type will be of type: logicalAddress R/W)

e. For a read, simply convert logical to physical and read the block contents(ie. add the time)

f. For a write, call the write() function

g. After every n writes(n defined by user in config file), invoke the dupFinder() followed by garbageCollector()

h. When a VM finishes processing all addresses from its input file, close the file

i. When files of all VMs get closed, shut down the server thread


## Description of mechanism of action from a Server's viewpoint
There are 3 main actions that a server performs:
### Handling a write request

(1) Convert the logical address to physical address. 

(2) If no physical address exists for that logical address, then
obtain a free block from the free queue and write to it.
Insert the entry for this block in the logical to physical
mapping. 
(3) If a physical address for the LBA already exists, then check
if it is COW.

(4) If not COW, then write it in-place.

(5) If the LBA is COW, then obtain a new block from the
free queue and write to it. Update the COW status and
logical to physical mapping of the LBA. Insert the address
of the old physical block corresponding to the LBA into
the unreferenced queue.

(6) Insert the block that was written to into the dirty queue.
 
### Out-of-line deduplication

(1) Get the block at the front of the dirty queue and carry out
logical to physical address conversion(say block A).

(2) Read the block and calculate its signature.

(3) Determine which DDI shard may contain that signature
and lock that particular shard .

(4) Query the DDI with the signature of the dirty block

(5) If there is no potential alias in the DDI, then insert a new
entry into the DDI corresponding to that signature.

(6) If there is potential alias(say block B), then mark B as
COW.

(7) The signature of B in the DDI may be stale. So read B and
calculate its signature.

(8) If signature of A and B are the same, then update the logical
to physical mapping for logical address. corresponding to
block A. Also increment the number of references to B in
the DDI. Put A in the free queue.

(9) If signature of A and B are not equal, then insert a new
entry into the DDI corresponding to signature of A. If
B was not previously marked COW, then remove COW
mark of B.

(10) Unlock the DDI shard.

### Garbage collection

(1) Get the next block address from the unreferenced queue(say
A).

(2) Read block A and calculate its signature.

(3) Determine which DDI shard may contain that signature
and lock that particular shard .

(4) Query the DDI shard with the signature of block A.

(5) If there is no entry in the DDI for the signature of A, then
put A in the free queue.

(6) If there is a matching entry for the signature of A(say
for block B), then check whether A and B are the same
physical address. If they are, then decrement the number
of references of B. If the number of references to B becomes
0, then remove its entry from the DDI and put it in the
free queue.

(7) Unlock the DDI shard.



