Forked from http://github.com/armon/libart
C++ wrapping,  C++11 primitives, Boost SmartPointers, threads and tests were used and also pfabric "core/Tuple.hpp"

 Running Solution on multi-core machine cloud01.prakinf.tu-ilmenau.de cloud:

1-  SSH into server with credentials with login id:
    ssh -p 2222  yourlogin@cloud01.prakinf.tu-ilmenau.de
2-  Clone a new MVCCART repository from  Github:
    https://github.com/umairsheikh/MVCCART
3-  Please Delete cmake build files, i.e.. cmake-build-debug, CMakeFiles, cmake_install.cmake, CMakeCache.txt and
    I am sorry for it needed to be ignored, i will do it i promise.

4- Rename CMakeLists_Production to CMakeLists.txt , written for multicore boost environment for prakinf.tu-ilmenau awesome cloud.
  boost install path is hardcoded and should be set accordingly with your boost install path in a file  CMakeFile_Production.txt,
  which is to be renamed ofcourse to make it functional. (Original CMake is for the dev environment i have on MACOSX, It works
  fine and CMakes auto finds boost as well)

5- Update the CMakeLists.txt file, and choose the desired Test CPP file, to run from WorkloadTests or from CorrectnessTests
  folder relative path.

6- In your MVCCART path, and execute command -> Cmake .
- Compile the code through CMAKE using command:
    cmake --build MVCCART/ --target mvccart -- -j 4
- Running the solution  ./mvccart
  ERROR:: "Please set path in your WorkloadTests/...CPP file chosen" to run. I need to automate this too. Path should be
  absolute path to test_data/words.txt" file. Here it it to be updated in a variable "rootpath_words"



NOTE:: Project is a part of Pipefabric. It has only depency on pfabric "core/Tuple.hpp" referenced from ART/ArtCPP.hpp so far. It can be
very easy for pfabricers to setup this project with complete pfabric branch too. Just Make Pfabric branch, and add this repository
under Pfabric branch, and use "Core/Tuple.hpp" from the pfabric complete branch.  However, It can be directly imported
in Clion and readily be used without any extra headaches of pfabric as well, since a limited code from pfabric is already imported
in this repository and depencies were resolved for "core/Tuple.hpp"



Your own Development setup with your own ART Instances can be used as follows:

1- Add following headers, in you pfabric project. I

 #include <iostream>
 #include <boost/test/unit_test.hpp>
 #include <boost/lexical_cast.hpp>
 #include "mvcc/mvcc.hpp"
 #include "ART/ARTFULCpp.h"
 //#include "Transactions/WriteOnlyTemplates.h" Use templates already defined. from Transaction folder.

2- Define globals to test and reuse typdef is handy.

 typedef pfabric::Tuple<string,unsigned long, int,string, double> RecordType;
 typedef char KeyType[20];
 typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
 char KeysToStore[235890][20];
 std::vector<RecordType> vectorValues;


2.1- Tuple schema is defined like: typedef pfabric::Tuple<std::string,unsigned long, int, std::string, double> RecordType;

2.2- test_data/Words.txt file, is imported and all the keys are stored in an array as KeysToStore

2,3- Vector<RecordType> is created, as a process to test and store values, in order to compare the tuple values after
inserting to ART and updating the tuple attributes while updating operation. Here is how tuples are generated.
While index is the iterative index, while reading values from words.txt. Similar to HashMapTableTest.cpp.

RecordType tuple = RecordType(buf,
                             (unsigned long) index,
                             index + 100,
                             fmt::format("String/{}", buf),
                             index / 100.0);

vectorValues.push_back(tuple);


3- Adaptive Radix Tree instance can be used as such: While ARTTupleContainer is defined in globals.
auto ARTable1 =  new ARTTupleContainer();

5- Transactions are created and can be seen directly from here as specimen.
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/
https://github.com/umairsheikh/MVCCART/tree/master/CorrectnessTests
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/


Parallelism/Concurrency control:

Based on my analysis of the MVCCART repository, here's how write parallelism and concurrency are carried out:

Key Concurrency Mechanisms
1. Transaction-Based Parallelism
The system uses a transaction management framework where multiple writers run in parallel:

umairsheikh / MVCCART / Transactions / transactionManager.h
v1
template <typename TransactionFunc, typename ARTContainer>
class Transaction
{
public:
    size_t  Tid;
    boost::thread* TransactionThread;
Each transaction gets its own Thread ID (Tid) and runs in a separate boost::thread.

2. Optimistic Locking with Version Tracking
The Adaptive Radix Tree (ART) nodes use version-based optimistic locking:

umairsheikh / MVCCART / ART / ArtCPP.hpp
v1
typedef struct
{
    uint8_t type;
    uint8_t num_children;
    uint32_t partial_len;
    unsigned char partial[MAX_PREFIX_LEN];
64-bit atomic version field with:
2 bits for type
60 bits for version number
1 bit for lock state
1 bit for obsolete flag
3. Compare-and-Swap (CAS) Operations
Write operations use atomic CAS to upgrade locks without traditional mutexes:

umairsheikh / MVCCART / ART / ArtCPP.hpp
v2
bool upgradeToWriteLockOrRestart(art_node* node, uint64_t& v, bool &needRestart)
{
    if (!node->version.compare_exchange_strong(v, v + 0b10)) 
    {
        v = v + 0b10;
        needRestart = true;
Writers attempt to acquire locks via CAS; if it fails, the transaction restarts.

4. MVCC (Multi-Version Concurrency Control)
The system maintains multiple versions of data:

umairsheikh / MVCCART / mvcc / snapshot.hpp
v1
template <class ValueType>
struct snapshot
{
    using value_type = ValueType;
    
    smart_ptr::shared_ptr<snapshot> _older_snapshot;
Each write creates a new snapshot linked to older versions, allowing readers to access consistent older versions while writers create new ones.

5. Global Epoch Management
Transactions are organized into epochs for coordinated visibility:

umairsheikh / MVCCART / Transactions / Epochs.hpp
class Epoch
{
public:
    int EpochId;
    std::vector<size_t> TxnSet;
    int counter;
6. Atomic Transaction ID Generation
Writers get unique transaction IDs atomically without blocking:

umairsheikh / MVCCART / Transactions / transactionManager.h
v2
size_t get_new_transaction_ID()
{
    size_t Tid;
    nextTid.fetch_add(1, boost::memory_order_relaxed);
    Tid = nextTid.load(boost::memory_order_relaxed);
    return Tid;
Uses boost::atomic with relaxed memory ordering for high performance.

Summary
Write parallelism is achieved through:

Lock-free transactions using optimistic locking and version numbers
Compare-and-swap instead of blocking locks
Multi-versioning allowing concurrent readers and writers
Epoch-based isolation for transaction coordination
Boost thread pool for parallel execution
This approach allows multiple writers to proceed concurrently without blocking on traditional mutexes, using restarts when conflicts occur—a key pattern in high-performance concurrent data structures.



