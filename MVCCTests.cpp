#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc.hpp"
#include "ARTFULCpp.h"
#include "transactionManager.h"

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <cassert>
#include "core/Tuple.hpp"
#include <boost/tuple/tuple.hpp>


using namespace std;


namespace
{
    auto hr_now() -> decltype(std::chrono::high_resolution_clock::now())
    {
        return std::chrono::high_resolution_clock::now();
    }

    string INIT = "init";
    string OVERWRITTEN = "overwritten";
    string UPDATED = "updated";
    string DISTURBED = "disturbed";
}



typedef pfabric::Tuple<unsigned long, int,string, double> RecordType;
typedef char KeyType[20];
typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
typedef std::function <void(ARTTupleContainer&,size_t id,std::string& status)> TableOperationOnTupleFunc;


char KeysToStore[250000][20];
std::vector<RecordType> mValue;


static  int cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    if(val != NULL)
    {

        mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
        //std::cout<<"###found K/V ="<<key<<"/"<<_mvvcValue->current()->value.getAttribute<1>()<<"/current="<<_mvvcValue->current()->version<<"\n";
        //if(_mvvcValue->current()->_older_snapshot != nullptr)
        //std::cout<<"/old="<<_mvvcValue->current()->_older_snapshot->version<<" / oldvalue="<<_mvvcValue->current()->_older_snapshot->value<<"\n";
    }
    return 0;
}


BOOST_AUTO_TEST_SUITE(MVCC_TESTS)

    BOOST_AUTO_TEST_CASE(test_from_loading_from_Buckets)
    {

        cout << "loading from buckets" << endl;
        int len, len2;
        char buf[20];
        char bufVal[50];

        /// Reading all Values to store against keys
        FILE *fvals = fopen("/Users/fuadshah/Desktop/MVCCART/test_data/uuid.txt", "r");
        FILE *fkeys = fopen("/Users/fuadshah/Desktop/MVCCART/test_data/words.txt", "r");
        int index = 0;

        while (fgets(bufVal, sizeof bufVal, fvals))
        {
            fgets(buf, sizeof buf, fkeys);
            len = strlen(bufVal);
            len2 = strlen(buf);
            buf[len2] = '\0';
            bufVal[len] = '\0';
            RecordType tuple = RecordType((unsigned long) index, index + 100, bufVal, index / 100.0);
            strcpy(KeysToStore[index] , buf);
            mValue.push_back(tuple);
            index++;

            if (index > 200000)
            {
                break;
            }
        }
    }

    BOOST_AUTO_TEST_CASE(test_null_snapshot_on_1st_reference)
    {
        std::cout<<"Runing test_null_snapshot_on_1st_reference"<<endl;
        mvcc11::mvcc<string> x;
        auto snapshot = *x;
        BOOST_CHECK(snapshot->version == 0);
        BOOST_CHECK(snapshot->value.empty() == true);
    }

    BOOST_AUTO_TEST_CASE(test_mvcc_snapshot_with_txnId_value_statuss)
    {
        std::cout<<"test_mvcc_snapshot_with_txnId_value_statuss"<<endl;

        string x = "excuse--me!!!";
        int i = 10;
        RecordType tuple =   RecordType((unsigned long) i, i + 100, x, i/100.0);
        mvcc11::mvcc<RecordType>* _mvcc = new mvcc11::mvcc<RecordType>(i,tuple,INIT);
        auto snapshot = _mvcc->current();

        BOOST_REQUIRE(snapshot->value.getAttribute<0>() == i);
        BOOST_REQUIRE(snapshot->value.getAttribute<1>() == i+100);
        BOOST_REQUIRE(snapshot->value.getAttribute<2>() == "excuse--me!!!");
        BOOST_REQUIRE(snapshot->value.getAttribute<3>() == i/100.0);

    }

    BOOST_AUTO_TEST_CASE(test_snapshot_isolation_and_old_snapshot)
    {
        cout << "test_snapshot_isolation_and_old_snapshot" << endl;
        int i = 1;
        RecordType tuple = RecordType((unsigned long) i, i + 100, INIT, i / 100.0);
        mvcc11::mvcc<RecordType> *_mvcc = new mvcc11::mvcc<RecordType>(i, tuple, INIT);
        auto snapshot = _mvcc->current();
        BOOST_REQUIRE(snapshot->version == 1);
        BOOST_REQUIRE(snapshot->value.getAttribute<2>() == INIT);

        i++;
        RecordType tuple2 = RecordType((unsigned long) i, i + 100, OVERWRITTEN, i / 100.0);
        _mvcc->overwriteMV(2, tuple2, OVERWRITTEN);
        auto snapshot1 = _mvcc->current();

        BOOST_REQUIRE(snapshot1->version == 2);
        BOOST_REQUIRE(snapshot1->value.getAttribute<2>() == OVERWRITTEN);
        BOOST_REQUIRE(snapshot1->_older_snapshot->version == 1);
        BOOST_REQUIRE(snapshot1->_older_snapshot->value.getAttribute<2>() == INIT);
    }

BOOST_AUTO_TEST_SUITE_END()
