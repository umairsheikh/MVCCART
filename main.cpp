#include <iostream>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "ARTFULCpp.h"
//#include "table/Table.hpp"
#include <boost/thread.hpp>
#include "core/Tuple.hpp"
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace pfabric;

void InsertAndIterateyTuples();


///Testing ART , with MyTuple for schema table type
typedef pfabric::Tuple<unsigned long, int, char *, double> MyTuple;
typedef TuplePtr<MyTuple> InTuplePointer;
typedef MyTuple TuplesToStore[50];
char ValuesToStore[10][50];
std::vector<MyTuple> Bucket;
std::vector<InTuplePointer> BucketOfPointers;


static  int iter_callbackByPredicate(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    //MyTuple * ptr = (MyTuple *)val;
    //MyTuple* ptr = reinterpret_cast<MyTuple*>(val);

    if(val != NULL)
    {
        //auto tptr = ptr->getAttribute<2>();
        int x = (uintptr_t)val;
        std::cout<<"###found K/V ="<<key<<"\n"<<BucketOfPointers[x]<<"\n";
        //std::cout<<"###found K/V ="<<key<<"\n"<<tptr<<"\n";
    }
    return 0;
}

static  int cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    //RecordType * ptr = (RecordType *)val;
    if(val != NULL)
    {
        std::cout<<"###found K/V ="<<key<<"/"<<(char *)val<<"\n";
    }
    return 0;
}

template <typename RecordType, typename KeyType>
void InsertOrUpdateFromFile(ARTFULCpp<RecordType, KeyType> myADTTable);

template <typename RecordType, typename KeyType = DefaultKeyType>
void InsertAndIterateByTuples(ARTFULCpp<RecordType, KeyType> testTable);

template <typename RecordType, typename KeyType>
void IterateByKeyValues(ARTFULCpp<RecordType, KeyType>& myADTTable);

template <typename RecordType, typename KeyType>
void IterateKeyValuesByPredicate(ARTFULCpp<RecordType, KeyType> myADTTable);

template <typename RecordType, typename KeyType>
void DeleteByKeyValue(ARTFULCpp<RecordType, KeyType> myADTTable);

template <typename RecordType, typename KeyType>
void SearchByKeyValue(ARTFULCpp<RecordType, KeyType> myADTTable);

void readerApi();

const int writerthreadcount = 0;
const int readerthreadcount = 0;

std::vector<boost::thread*> writerthread;
std::vector<boost::thread*> readerthread;


int main()
{
    std::cout << "Adaptive Radix Tree " << std::endl;


    ///  Create template for ARTTable. simple template Type
    /// of Index Page Char[50] RecordType Char[20] KeyTyp
    auto ARTable =  new ARTFULCpp<char[50], char[20]>();
    //auto ARTable =  new ARTFULCpp<uintptr_t, char[20]>();

    //ARTFULCpp<char[50], char[20]> ARTable =   ARTFULCpp<char[50], char[20]>();
    /// Insert or Update Tuple from file
   InsertOrUpdateFromFile<char [50] ,char[20]>(*ARTable);



    /// Iterate all Tuples and get callback
    /// for every tuple found against keys in ART Index Page
    //IterateByKeyValues<char[50] ,char[20]>(*ARTable);

    /// Iterate Tuple by predicate in ART Index Page
    ///Gets callback only if predicate satisfies
    //IterateKeyValuesByPredicate<char[50] ,char[20]>(*ARTable);

    /// Delete Tuple by Key Gets value deleted
    /// only if key exists Exists
    //DeleteByKeyValue<char[50] ,char[20]>(*ARTable);

    /// Look for tuple by its Key
    //SearchByKeyValue<char[50] ,char[20]>(*ARTable);


    //InsertAndIterateByTuples<uintptr_t,char[20]>(*ARTable);
    ARTable->DestroyAdaptiveRadixTreeTable();
    //usleep(2000);
    std::cout<<"Completed Successfully!!";
    return 0;
}


void readerApi()
{
    for (int i=0; i < 10; i++) {
        usleep(400);
        std::cout << "readerApi: " << i
                  << std::endl;
    }
}

template <typename RecordType, typename KeyType>
void InsertOrUpdateFromFile(ARTFULCpp<RecordType, KeyType> myADTTable)
{
    int len, len2;
    char buf[20];
    char bufVal[50];


    /* using MyVector = std::vector<char *>;
    auto v1 = std::vector<std::shared_ptr<IntVector>> {
            std::make_shared<IntVector>(IntVector{1,2,3}),
            std::make_shared<IntVector>(IntVector{4,5,6}),
            std::make_shared<IntVector>(IntVector{7,8,9})
    };*/

    /// Reading all Values to store against keys
    FILE *fvals = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/uuid.txt", "r");
    FILE *f = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    int index = 0;
    int line = 0;
    while (fgets(bufVal, sizeof bufVal, fvals)) {
        fgets(buf, sizeof buf, f);
        len = strlen(bufVal);
        len2 = strlen(buf);

        buf[len2] = '\0';
        bufVal[len] = '\0';
        char *temp = bufVal;
        strcpy(ValuesToStore[index], bufVal);
        cout << "\ninserting key= " <<buf<<"  - value = "<<ValuesToStore[index]<<endl;
        myADTTable.insertOrUpdateByKey(buf, ValuesToStore[index]);

        //cout<<index<<" tuple added::::::"<<ValuesToStore[index]<<"\n";
        //strcpy(ValuesToStore[index], bufVal);
        index++;
        if (index == 10)
            break;
    }
}

template <typename RecordType, typename KeyType>
void IterateByKeyValues(ARTFULCpp<RecordType, KeyType>& myADTTable)
{
    myADTTable.iterate(cb);

}

template <typename RecordType, typename KeyType>
void IterateKeyValuesByPredicate(ARTFULCpp<RecordType, KeyType> myADTTable)
{
    int len, len2;
    char buf[20];
    char bufVal[50];



    ///select all
    std::function<bool(void *)> predAll = [](void* R){return true;};
    std::function<bool(void *)> pred = [](void* R){RecordType dummy; strcpy(dummy,(char*)R);return dummy[0]=='7';};
    myADTTable.iterateByPredicate(cb,pred);
    myADTTable.DestroyAdaptiveRadixTreeTable();
    cout<<"Exited normaly";
}

template <typename RecordType, typename KeyType>
void SearchByKeyValue(ARTFULCpp<RecordType, KeyType> myADTTable)
{
    int len, len2;
    char buf[20];
    char bufVal[50];



    FILE *f2 = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    //std::cout << "Searching the Keys now..." << std::endl;
    int line = 0;
    while (fgets(buf, sizeof buf, f2))
    {
        len = strlen(buf);
        buf[len] = '\0';
        //std::cout << "\nKey To Find ::::" << buf << std::endl;

        RecordType * gotval = myADTTable.findValueByKey(buf);
        if(gotval)
        std::cout<<"\nkey ="<<buf<<" foundvalue "<<(char*)gotval<<"\n";
        line++;
        usleep(10);

        if (line == 10)
            break;
    }

}

template <typename RecordType, typename KeyType>
void DeleteByKeyValue(ARTFULCpp<RecordType, KeyType> myADTTable)
{


    int len, len2;
    char buf[20];
    char bufVal[50];

    FILE *f2 = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    cout << "Deleting the Keys now..." << endl;
    int line = 0;
    while (fgets(buf, sizeof buf, f2))
    {
        len = strlen(buf);
        buf[len] = '\0';
        cout << "Key To Delete ::::" << buf << endl;
        RecordType * val = myADTTable.deleteByKey(buf);
        cout << "Size of ART ::::" << myADTTable.getARTSize()<<"\n";
        if(val != NULL)
            cout<<"  Value Deleted="<< *val<<"\n";
        line++;
        if (line == 10)
            break;
    }

}

template <typename RecordType, typename KeyType>
void InsertAndIterateByTuples(ARTFULCpp<RecordType, KeyType> testTable)
{

    // auto  testTable = std::make_shared<ARTable<InTuplePointer,DefaultKeyType>> ();
    int len, len2;
    char buf[20];
    char bufVal[50];
    FILE *fvals = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    FILE *uuid = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/uuid.txt", "r");
    uintptr_t index = 0;
    int i=1;


    while (fgets(buf, sizeof buf, fvals))
    {
        fgets(bufVal, sizeof bufVal, uuid);

        len  = strlen(buf);
        len2 = strlen(bufVal);
        bufVal[len2]='\0';
        buf[len] = '\0';
        strcpy(ValuesToStore[index], bufVal);
        //Bucket.push_back(MyTuple((unsigned long) i, i + 100, ValuesToStore[index], i / 100.0));
        auto tup = makeTuplePtr((unsigned long) i, i + 100,ValuesToStore[index] , i / 100.0);
        BucketOfPointers.push_back(tup);

        cout<<" typed key / val = "<<buf<<"/"<<BucketOfPointers.at(index)<<endl;

        //InTuplePointer tptr (new MyTuple((unsigned long) i, i + 100, bufVal, i / 10f0.0));
        testTable.insertOrUpdateByKey(buf,index);
        i++;
        index++;

        if(i > 10)//235887)
            break;
    }

    testTable.iterate(iter_callbackByPredicate);

}
