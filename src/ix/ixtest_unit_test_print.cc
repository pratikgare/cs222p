#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ix.h"
#include "ix_test_util.h"

IndexManager *indexManager;

// Required set functions
RC setLeafFlag(void* data, short value){
	// 2-4 bytes
	int offset = 2;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC setEntryCount(void* data, short value){
	// 6-8 bytes
	int offset = 6;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC setPointerToLeft(void* data, short value){
	// 8-10 bytes
	int offset = 8;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC setPointerToRight(void* data, short value){
	// 10-12 bytes
	int offset = 10;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

void createLeafRecord(void* page, int offset, int key, int ridpagenum, int ridslotnum){
	memcpy((char*)page+offset, &key, sizeof(int));
	offset += sizeof(int);
	memcpy((char*)page+offset, &ridpagenum, sizeof(int));
	offset += sizeof(int);
	memcpy((char*)page+offset, &ridslotnum, sizeof(int));
}

void createNonLeafRecord(void* page, int offset, int key, short leftpagenum, short rightpagenum){
	memcpy((char*)page+offset, &leftpagenum, sizeof(short));
	offset += sizeof(short);
	memcpy((char*)page+offset, &key, sizeof(int));
	offset += sizeof(int);
	memcpy((char*)page+offset, &rightpagenum, sizeof(short));
}



RC testCase_1(const string &indexFileName)
{
    // Functions tested
    // 1. Create Index File **
    // 2. Open Index File **
    // 3. Create Index File -- when index file is already created **
    // 4. Open Index File ** -- when a file handle is already opened **
    // 5. Close Index File **
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Unit Test Case Print *****" << endl;

    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");

    // open index file
    IXFileHandle ixfileHandle;
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");


    // - Create our own B-Tree

    // Setting root page number
    ixfileHandle.fileHandle.setCounter(5,4);

    void* page1 = malloc(PAGE_SIZE);
    setEntryCount(page1,3);
    setPointerToLeft(page1, -1);
    setPointerToRight(page1, -1);
    setLeafFlag(page1, 1);

    int offset1 = 30;
    for(int i = 0; i<3 ; i++){
    		if(i==0)
    			createLeafRecord(page1, offset1, 36, 2, 4);
    		if(i==1)
			createLeafRecord(page1, offset1, 36, 2, 5);
    		if(i==2)
			createLeafRecord(page1, offset1, 37, 3, 8);

    		offset1 += 12;
    }
    ixfileHandle.fileHandle.appendPage(page1);



    void* page2 = malloc(PAGE_SIZE);
    setEntryCount(page2,1);
    setPointerToLeft(page2, -1);
    setPointerToRight(page2, -1);
    setLeafFlag(page2, 1);
    int offset2 = 30;
    	createLeafRecord(page2, offset2, 37, 1, 2);
    ixfileHandle.fileHandle.appendPage(page2);

    void* page3 = malloc(PAGE_SIZE);
    setEntryCount(page3,1);
    setPointerToLeft(page3, -1);
    setPointerToRight(page3, -1);
    setLeafFlag(page3, -1);
    int offset3 = 30;
    	createNonLeafRecord(page3, offset2, 37, 1, 2);
    ixfileHandle.fileHandle.appendPage(page2);


    // close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");

    return success;
}

int main()
{
    // Global Initialization
    indexManager = IndexManager::instance();

    const string indexFileName = "age_idx";
    remove("age_idx");

    RC rcmain = testCase_1(indexFileName);
    if (rcmain == success) {
        cerr << "***** IX Test Case 1 finished. The result will be examined. *****" << endl;
        return success;
    } else {
        cerr << "***** [FAIL] IX Test Case 1 failed. *****" << endl;
        return fail;
    }
}

