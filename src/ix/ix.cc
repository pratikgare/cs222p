
#include "ix.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>

// My macros flag
#define NOT_DELETED 0
#define LEAF_FLAG 1
#define NO_ROOT -1
#define METADATA 30
#define EMPTY_PAGE PAGE_SIZE-METADATA
#define DEFAULT_VAL -1

using namespace std;


IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->createFile(fileName);
}

RC IndexManager::destroyFile(const string &fileName)
{
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->destroyFile(fileName);
}

RC IndexManager::openFile(const string &fileName, IXFileHandle &ixfileHandle)
{
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->openFile(fileName, ixfileHandle.fileHandle);
}

RC IndexManager::closeFile(IXFileHandle &ixfileHandle)
{
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->closeFile(ixfileHandle.fileHandle);
}

// Added functions

RC setDeletedFlag(void* data, short value){
	// 0-2 bytes
	int offset = 0;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getDeletedFlag(void* data, short &value){
	// 0-2 bytes
	int offset = 0;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setLeafFlag(void* data, short value){
	// 2-4 bytes
	int offset = 2;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getLeafFlag(void* data, short &value){
	// 2-4 bytes
	int offset = 2;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setTreeLevel(void* data, short value){
	// 4-6 bytes
	int offset = 4;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getTreeLevel(void* data, short &value){
	// 4-6 bytes
	int offset = 4;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setEntryCount(void* data, short value){
	// 6-8 bytes
	int offset = 6;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getEntryCount(void* data, short &value){
	// 6-8 bytes
	int offset = 6;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC incrementEntryCount(void* data){
	short value = 0;
	getEntryCount(data, value);
	value++;
	setEntryCount(data, value);
	return 0;
}

RC decrementEntryCount(void* data){
	short value = 0;
	getEntryCount(data, value);
	value--;
	setEntryCount(data, value);
	return 0;
}

RC setPointerToLeft(void* data, short value){
	// 8-10 bytes
	int offset = 8;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getPointerToLeft(void* data, short &value){
	// 8-10 bytes
	int offset = 8;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setPointerToRight(void* data, short value){
	// 10-12 bytes
	int offset = 10;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getPointerToRight(void* data, short &value){
	// 10-12 bytes
	int offset = 10;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setPointerToOverflow(void* data, short value){
	// 12-14 bytes
	int offset = 12;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getPointerToOverflow(void* data, short &value){
	// 12-14 bytes
	int offset = 12;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

int setFreeSpace(void* data, short value){
	// 14-16 bytes
	int offset = 14;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getFreeSpace(void* data, short &value){
	// 14-16 bytes
	int offset = 14;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC incrementFreeSpace(void* data, int value){
	short freeSpace = 0;
	getFreeSpace(data, freeSpace);
	freeSpace += value;
	setFreeSpace(data, freeSpace);
	return 0;
}

RC decrementFreeSpace(void* data, int value){
	short freeSpace = 0;
	getFreeSpace(data, freeSpace);
	freeSpace -= value;
	setFreeSpace(data, freeSpace);
	return 0;
}

RC setLeftPageNum(void* data, short value){
	// 16-18 bytes
	int offset = 16;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getLeftPageNum(void* data, short &value){
	// 16-18 bytes
	int offset = 16;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

RC setOwnPageNumber(void* data, short value){
	// 18-20 bytes
	int offset = 18;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getOwnPageNumber(void* data, short &value){
	// 18-20 bytes
	int offset = 18;
	memcpy(&value, (char*)data + offset, 2);
	return 0;
}

// Creating page meta data design - till now 30 bytes
RC initializePage(void* data){
	/*
	 bytes: 0-2 : deletion_flag ( deleted = 0; else non-deleted page)
	 bytes: 2-4 : leaf/non-leaf flag (leaf_page = 0; else non_leaf page
	 bytes: 4-6 : level of the page in tree
	 bytes: 6-8 : number of entries in this page
	 bytes: 8-10: pointer to left sibling (unsigned page num)
	 bytes: 10-12: pointer to right sibling (unsigned page num)
	 bytes: 12-14: pointer to overflow pages (unsigned page num)
	 bytes: 14-16: free space in the given page
	 bytes: 16-18: store pageNum as the left ptr for non-leaf nodes
	 bytes: 18-20: Own page's pagenumber
	 bytes: 20-30: buffer (unused)
	*/
	short defaultValue = -1;

	//flag to check whether a page is empty or not (thanks to lazy delete)
	//initialized to -1
	setDeletedFlag(data, NOT_DELETED);

	//flag to check whether a page is leaf or not
	//Assumption: newly created node is a TODO:check with RAJAT
	setLeafFlag(data, LEAF_FLAG);

	short defaultValue2 = 0;

	//Value which will give us page location wrt to tree level
	setTreeLevel(data, defaultValue2);

	//Value which will give us total entries in the given page
	setEntryCount(data, defaultValue2);

	//Useful if we are implementing a doubly linked list, else useless
	setPointerToLeft(data, defaultValue);

	//Necessary to implement a single linked list
	setPointerToRight(data, defaultValue);

	//Overflow page to handle duplicate values which do not fit into a single page
	setPointerToOverflow(data, defaultValue);

	//add freespace entry
	short freeSpace = 4096-METADATA;
	setFreeSpace(data, freeSpace);

	//unit test case to test free space getter
	short retVal = 0;
	getFreeSpace(data, retVal);

	//set right page ptr for non-leaf pages
	setLeftPageNum(data, defaultValue);

	//unit test case to test right page ptr
	getLeftPageNum(data, retVal);

	return 0;
}

//Prepare leaf entry
// Has only key and RID
RC prepareLeafEntry(const void* indexKey, void* indexEntry, AttrType type, const RID &rid, short &entrySize){

	switch(type){
		case TypeInt:{
			//indexEntry = malloc(sizeof(int)+sizeof(RID));

			int offset = 0;
			memcpy((char*)indexEntry+offset, indexKey, sizeof(int));
			offset += sizeof(int);

			memcpy((char*)indexEntry+offset, &rid, sizeof(RID));
			offset += sizeof(RID);

			entrySize = offset;
			break;
		}
		case TypeReal:{
			//indexEntry = malloc(sizeof(int)+sizeof(RID));

			int offset = 0;
			memcpy((char*)indexEntry+offset, indexKey, sizeof(int));
			offset += sizeof(int);

			memcpy((char*)indexEntry+offset, &rid, sizeof(RID));
			offset += sizeof(RID);

			entrySize = offset;
			break;
		}
		case TypeVarChar:{

			int varCharSize = 0;

			memcpy(&varCharSize, indexKey, sizeof(int));

			//indexEntry = malloc(sizeof(int) + varCharSize + sizeof(RID));

			int offset = 0;
			memcpy(indexEntry, indexKey, sizeof(int) + varCharSize);

			offset += (sizeof(int) + varCharSize);

			memcpy((char*)indexEntry+offset, &rid, sizeof(RID));
			offset += sizeof(RID);

			entrySize = offset;
			break;
		}
	}

	return 0;
}

//Prepare non-leaf entry
// Has a key a right page pointer
RC prepareNonLeafEntry(const short rightChildPageNum, const void* indexKey, void* indexEntry, AttrType type, short &entrySize){

	switch(type){
		case TypeInt:{
			int offset = 0;

			//key
			memcpy((char*)indexEntry+offset, indexKey, sizeof(int));
			offset += sizeof(int);

			//right pagenum
			memcpy((char*)indexEntry+offset, &rightChildPageNum, sizeof(short));
			offset += sizeof(short);

			entrySize = offset;
			break;
		}
		case TypeReal:{
			int offset = 0;

			//key
			memcpy((char*)indexEntry+offset, indexKey, sizeof(float));
			offset += sizeof(float);

			//right pagenum
			memcpy((char*)indexEntry+offset, &rightChildPageNum, sizeof(short));
			offset += sizeof(short);

			entrySize = offset;
			break;
		}
		case TypeVarChar:{
			int varCharSize = 0;

			memcpy(&varCharSize, indexKey, sizeof(int));

			int offset = 0;

			//key
			memcpy(indexEntry, indexKey, sizeof(int) + varCharSize);
			offset += (sizeof(int) + varCharSize);

			//right pagenum
			memcpy((char*)indexEntry+offset, &rightChildPageNum, sizeof(short));
			offset += sizeof(short);

			entrySize = offset;
			break;
		}
	}

	return 0;
}





RC extractKey(const void* page, int offset, AttrType type, void* key, int &keyLength){

	switch(type){
		case TypeInt:{
			keyLength = sizeof(int);
			memcpy(key, (char*)page+offset, keyLength);
			break;
		}

		case TypeReal:{
			keyLength = sizeof(float);
			memcpy(key, (char*)page+offset, keyLength);
			break;
		}

		case TypeVarChar:{
			memcpy(&keyLength, (char*)page+offset, sizeof(int));
			memcpy(key, (char*)page+offset, sizeof(int)+keyLength);
			keyLength += sizeof(int);
			break;
		}
	}

	return 0;
}

// compare key function
RC compareKey(const void* key1, const void* key2, AttrType type){

	int status = 0;

	switch(type){
		case TypeInt:{

			int k1 = 0;
			memcpy(&k1, key1, sizeof(int));

			int k2 = 0;
			memcpy(&k2, key2, sizeof(int));

			if(k1 == k2){
				return 0;
			}
			else if(k1>k2){
				return 1;
			}
			else{
				return -1;
			}
			break;
		}

		case TypeReal:{
			float k1 = 0.0;
			memcpy(&k1, key1, sizeof(float));

			float k2 = 0;
			memcpy(&k2, key2, sizeof(float));

			if(k1 == k2){
				return 0;
			}
			else if(k1>k2){
				return 1;
			}
			else{
				return -1;
			}
			break;
		}

		case TypeVarChar:{
			int keyLength = 0;
			memcpy(&keyLength, key1, sizeof(int));

			char* varcharkey1 = (char*) malloc(keyLength+1);
			memcpy(varcharkey1, (char*)key1+sizeof(int), keyLength);
			varcharkey1[keyLength] = '\0';

			memcpy(&keyLength, key2, sizeof(int));
			char* varcharkey2 = (char*) malloc(keyLength+1);
			memcpy(varcharkey2, (char*)key2+sizeof(int), keyLength);
			varcharkey2[keyLength] = '\0';

			status = strcmp(varcharkey1, varcharkey2);

			break;
		}

	}

	return status;
}



//Should return the place where it should insert
//TODO: irrespective of page type which will insert/give position to be inserted
RC getOffset(void* page, const void* searchKey, int &keyLength, AttrType type, int &offsetToBeInserted){

	offsetToBeInserted = METADATA;

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	short leaf = 0;
	getLeafFlag(page, leaf);


	bool isLeaf = (leaf==LEAF_FLAG) ? true : false;

	void* key = malloc(PAGE_SIZE);

	//search for the correct position
	for(int i=0; i<entryCount; i++){
		extractKey(page, offsetToBeInserted, type, key, keyLength);

		if(compareKey(key, searchKey, type) >= 0){
			break;
		}

		if(isLeaf){
			offsetToBeInserted += keyLength + sizeof(RID);
		}
		else{
			offsetToBeInserted += keyLength + sizeof(short);
		}


	}

	return 0;
}

RC shiftRightAndInsertEntry(void* page, const void* entry, const int &entrySize, const int &offsetToBeInserted){

	void* buffer = malloc(PAGE_SIZE);

	// copy everything before offsetToBeInserted
	memcpy(buffer, page, offsetToBeInserted);

	//inserting the entry
	memcpy((char*)buffer+offsetToBeInserted, entry, entrySize);

	// copy everything after offsetToBeInserted
	memcpy((char*)buffer+offsetToBeInserted+entrySize, (char*)page+offsetToBeInserted, PAGE_SIZE-offsetToBeInserted-entrySize);

	// write back
	memcpy(page, buffer, PAGE_SIZE);

	free(buffer);



	return 0;

}

RC writeIntoPageBuffer(void* page, const void* entry, const int &entrySize, const int &offsetToBeInserted){

	//int testKey = 0;
	//memcpy(&testKey, entry, sizeof(int));
	//cout << "After calling : " << testKey <<endl;

	shiftRightAndInsertEntry(page, entry, entrySize, offsetToBeInserted);

	//updating the metadata
	short value = 0;

	// update number of entries
	getEntryCount(page, value);
	//cout << "Before entrycount: " << value << endl;
	value++;
	setEntryCount(page, value);
	//cout << "After entrycount: " << value << endl;

	// update freespace
	getFreeSpace(page, value);
	//cout << "Before freespace: " << value << endl;
	value -= entrySize;
	setFreeSpace(page, value);
	//cout << "After freespace: " << value << endl;

	return 0;
}

//called iff there is free space
RC writeIntoPage(IXFileHandle &ixfileHandle, void* page_data, short pageNumber){
	ixfileHandle.fileHandle.writePage(pageNumber, page_data);
	return 0;
}

//RC insertEntryIntoGivenPage(void* page_data, const void* entry, const int entrySize, int offsetToBeInserted){
//
//	//add entry in a page iff there is space
//
//	//write into buffer - shift right to create space and insert entry in sorted location
//	writeIntoPageBuffer(page_data, entry, entrySize, offsetToBeInserted);
//
//	else{
//
//		//split
//
//
//		//write into a leaf node
//
//
//
//		//update the metadata i.e decrement the freespace
//
//
//		//write into a non-leaf node = ROOT CREATION
//		//replace the root pointer in the hidden page - call set with mode 5
//
//
//		//update the metadata i.e decrement the freespace
//
//	}
//
//	return 0;
//}



RC getSplitOffset(void* page, void* key, int &keyLength, AttrType type, int &offsetToBeInserted, int &visitedKeyCount){

	offsetToBeInserted = METADATA;

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	short leaf = 0;
	getLeafFlag(page, leaf);


	int threshold = (PAGE_SIZE-METADATA)/2;

	bool isEOF = true;

	bool isLeaf = (leaf==LEAF_FLAG) ? true : false;

	void* prevKey = malloc(PAGE_SIZE);

	//search for the correct position
	for(int i=0; i<entryCount; i++){

		extractKey(page, offsetToBeInserted, type, key, keyLength);

		if(offsetToBeInserted >= threshold){
			if(i!=0){
				if(compareKey(prevKey, key, type) != 0){
					isEOF = false;
					break;
				}
			}
		}

		if(isLeaf){
			offsetToBeInserted += keyLength + sizeof(RID);
			visitedKeyCount++;
		}
		else{
			offsetToBeInserted += keyLength + sizeof(short);
			visitedKeyCount++;
		}

		memcpy(prevKey, key, keyLength);

	}

	if(isEOF){
		//find the duplicate origin
		getOffset(page, key, keyLength,type, offsetToBeInserted);
	}

	return 0;
}

RC printKey(void* key, const int keylength, AttrType type){
	switch(type){
		case TypeInt:{
			int keyvalue = 0;
			memcpy(&keyvalue, key, keylength);
			cout << keyvalue;
			break;
		}
		case TypeReal:{
			float keyvalue = 0.0;
			memcpy(&keyvalue, key, keylength);
			cout << keyvalue;
			break;
		}
		case TypeVarChar:{
			char *keyvalue = (char*) malloc(keylength+1);
			memcpy(&keyvalue, key, keylength);
			keyvalue[keylength] = '\0';
			cout << keyvalue;
			break;
		}
	}

	return 0;
}

RC printNonLeafPageEntries(void* page_data, AttrType type){

	// First key starts from this position, Metadata + leftChild
	int offset = METADATA + sizeof(short);

	short entry_count = 0;
	getEntryCount(page_data, entry_count);

	cout<<"\"keys\":[";

	// Printing all the keys only, Non-Leaf page, no duplicates handling
	void* key = malloc(PAGE_SIZE);
	int keyLength = 0;
	for(int i=0; i<entry_count; i++){
		// extract the key from the desired offset
		extractKey(page_data, offset, type, key, keyLength);
		cout<<"\"";

		// print the key based upon the attribute type
		printKey(key, keyLength, type);
		cout<<"\"";

		// Just skip the last comma
		if(i != entry_count-1){
			cout<<",";
		}

		// increase the offset such that next key can be extracted in next iteration
		// key + right child
		offset = keyLength + sizeof(short);
	}
	cout<<"]";

	return 0;
}



RC printLeafPageEntries(void* page_data, AttrType type){

	// First <key,rid> starts from this position
	int offset = METADATA;

	bool dupli_flag = false;

	short entry_count = 0;
	getEntryCount(page_data, entry_count);

	cout << "\"keys\":[";

	// Printing all the <key,rid> pair, Leaf page, duplicates handling too
	void* key = malloc(PAGE_SIZE);
	void* prev_key = malloc(PAGE_SIZE);
	RID rid;
	int keyLength = 0;

	for(int i=0; i<entry_count; i++){
		// extract the key from the desired offset
		extractKey(page_data, offset, type, key, keyLength);

		// Check for duplication after skipping the first iteration
		if(compareKey(key, prev_key, type) == 0 && i!=0){
			dupli_flag = true;
			cout << ",";
		}
		else{
			if(i!=0){
				dupli_flag = false;
				cout << "]\"";
				// Just skip the last comma
				if(i != entry_count-1 ){
					cout<<",";
				}
			}
		}

		if(dupli_flag == false){
			cout << "\"";
			// print the key based upon the attribute type
			printKey(key, keyLength, type);
			cout << ":[";
		}

		// Extract RID and print it
		memcpy(&rid, (char*)page_data + offset + keyLength, sizeof(RID));
		cout << "(" << rid.pageNum << "," << rid.slotNum << ")";

		// last entry printing
		if(i == entry_count - 1){
			cout << "]\"";
			break;
		}

		// increase the offset such that next key can be extracted in next iteration
		// key + right child
		offset = keyLength + sizeof(short);

		// Setting prev_key as currentkey for next iteration of duplicate check
		memcpy(prev_key, key, keyLength);
	}
	cout<<"]";

	return 0;
}

RC printPageEntries(void* page, AttrType type, short pageNum){

	short leaf_flag = 0;
	getLeafFlag(page, leaf_flag);

	bool isLeaf = (leaf_flag == LEAF_FLAG) ? true : false;

	if(isLeaf){
		short entryCount = 0;
		getEntryCount(page, entryCount);

		// empty page/deleted page
		if(entryCount < 1){
			// TODO: What if the Leaf is deleted?
			// probably return 1 to signal this??? How to Handle?
			return 1;
		}
		else{
			printLeafPageEntries(page, type);
		}
	}
	else{
		printNonLeafPageEntries(page, type);
	}

	return 0;

}

//TODO: complete the function
RC printMyPageEntries(void* page, AttrType type){

	short currPgNum = 0;
	getOwnPageNumber(page, currPgNum);


	short leaf = 0;
	getLeafFlag(page, leaf);

	bool isLeaf = (leaf == LEAF_FLAG)? true : false;

	cout<<"Pg num : "<<currPgNum<<endl;

	if(!isLeaf){
		short leftPgNum = 0;
		getLeftPageNum(page, leftPgNum);
		cout<< leftPgNum << " ";
	}

	switch(type){
		case TypeInt:{
			short recordCount;
			getEntryCount(page, recordCount);
			int offset = METADATA;

			for(int i=0;i<recordCount;i++){
				int val;
				memcpy(&val, (char*) page + offset, sizeof(int));
				cout << val << " ";
				if(isLeaf){
					offset+=sizeof(int) + sizeof(RID);
				}
				else{
					short rightPgNum = 0;
					memcpy(&rightPgNum, (char*)page+offset+sizeof(int), sizeof(short));
					cout<<rightPgNum<<" ";
					offset+=sizeof(int) + sizeof(short);
				}
			}
			cout<<"-----" << endl;
			break;
		}
		case TypeReal:{

			break;
		}
		case TypeVarChar:{

			break;
		}

	}


	return 0;
}

RC splitNonLeaf(IXFileHandle &ixfileHandle, void* oldPage, const void* key, AttrType type, const void* entry, const int entrySize, void* pushKey, short &pushVal){
	int splitOffset = 0;

	//get the split offset and pushKey
	int visitedKeyCount = 0;
	int pushKeyLength = 0;
	getSplitOffset(oldPage, pushKey, pushKeyLength, type, splitOffset, visitedKeyCount);

	void* newPage = malloc(PAGE_SIZE);
	initializePage(newPage);

	//set as nonleaf page
	setLeafFlag(newPage, DEFAULT_VAL);

	// put entries in new page
	int newOffset = splitOffset+pushKeyLength+sizeof(short);

	//leftPageNum
	short leftPgNum = 0;
	memcpy(&leftPgNum, (char*)oldPage+newOffset-sizeof(short), sizeof(short));
	setLeftPageNum(newPage, leftPgNum);

	//update the pushKey
	memcpy(pushKey, (char*)oldPage+splitOffset, pushKeyLength);

	// freespace
	short oldPageFreeSpace = 0;
	getFreeSpace(oldPage, oldPageFreeSpace);

	short blockSize = (PAGE_SIZE - oldPageFreeSpace - newOffset);

	memcpy((char*)newPage+METADATA, (char*)oldPage+newOffset, blockSize);

	// -update metadata for new page
	decrementFreeSpace(newPage, blockSize);

	// entryCount of old page
	short oldPageEntryCount = 0;
	getEntryCount(oldPage, oldPageEntryCount);

	//sub 1 for the deleted pushed key
	short newPageEntryCount = oldPageEntryCount - visitedKeyCount - 1;
	setEntryCount(newPage, newPageEntryCount);

	//ownPgNum
	short newPageNum = (short)ixfileHandle.fileHandle.getNumberOfPages();
	setOwnPageNumber(newPage, newPageNum);

	//pushVal
	pushVal = newPageNum;

	//delete the redundant entries
	void* buffer = malloc(PAGE_SIZE);
	memcpy(buffer, oldPage, splitOffset);
	memcpy(oldPage, buffer, PAGE_SIZE);

	//update metadata old page
	setEntryCount(oldPage, (short)visitedKeyCount);
	incrementFreeSpace(oldPage, blockSize+pushKeyLength+sizeof(short));
	setLeafFlag(oldPage, DEFAULT_VAL);

	//insert the entry into respective pages
	if(compareKey(key, pushKey, type) >= 0){
		//insert in new page
		int keylen = 0;
		int offset = 0;
		getOffset(newPage, key, keylen, type, offset);
		shiftRightAndInsertEntry(newPage, entry, entrySize, offset);

		decrementFreeSpace(newPage, entrySize);
		incrementEntryCount(newPage);
	}
	else{
		//insert in old page
		int keylen = 0;
		int offset = 0;
		getOffset(oldPage, key, keylen, type, offset);
		shiftRightAndInsertEntry(oldPage, entry, entrySize, offset);

		decrementFreeSpace(oldPage, entrySize);
		incrementEntryCount(oldPage);
	}

	//write the page to disk
	ixfileHandle.fileHandle.appendPage(newPage);

	short oldPageNum = 0;
	getOwnPageNumber(oldPage, oldPageNum);
	ixfileHandle.fileHandle.writePage(oldPageNum, oldPage);

	return 0;
}

RC splitLeaf(IXFileHandle &ixfileHandle, void* oldPage, const void* key, AttrType type, const void* entry, const int entrySize, void* pushKey, short &pushVal){

	//
	//printMyLeafEntries(oldPage, type);


	int splitOffset = 0;

	//get the split offset and pushKey
	int visitedKeyCount = 0;
	int pushKeyLength = 0;
	getSplitOffset(oldPage, pushKey, pushKeyLength, type, splitOffset, visitedKeyCount);

	void* newPage = malloc(PAGE_SIZE);
	initializePage(newPage);

	// freespace
	short oldPageFreeSpace = 0;
	getFreeSpace(oldPage, oldPageFreeSpace);

	short blockSize = (PAGE_SIZE - oldPageFreeSpace - splitOffset);

	// put entries in new page
	memcpy((char*)newPage+METADATA, (char*)oldPage+splitOffset, blockSize);

	// -update metadata for new page
	decrementFreeSpace(newPage, blockSize);

	// entryCount of old page
	short oldPageEntryCount = 0;
	getEntryCount(oldPage, oldPageEntryCount);

	//
	short newPageEntryCount = oldPageEntryCount - visitedKeyCount;
	setEntryCount(newPage, newPageEntryCount);

	//own pg num
	short newPageNum = (short)ixfileHandle.fileHandle.getNumberOfPages();
	setOwnPageNumber(newPage, newPageNum);

	//pushVal
	pushVal = newPageNum;

	//Linked list ptrs
	short oldPagePtrToRight = 0;
	getPointerToRight(oldPage, oldPagePtrToRight);
	setPointerToRight(newPage, oldPagePtrToRight);



	//delete the redundant entries
	void* buffer = malloc(PAGE_SIZE);
	memcpy(buffer, oldPage, splitOffset);
	memcpy(oldPage, buffer, PAGE_SIZE);

	//update metadata old page
	setEntryCount(oldPage, (short)visitedKeyCount);
	incrementFreeSpace(oldPage, blockSize);
	setPointerToRight(oldPage, newPageNum);

	//insert the entry into respective pages
	if(compareKey(key, pushKey, type) >= 0){
		//insert in new page
		int keylen = 0;
		int offset = 0;
		getOffset(newPage, key, keylen, type, offset);
		shiftRightAndInsertEntry(newPage, entry, entrySize, offset);

		decrementFreeSpace(newPage, entrySize);
		incrementEntryCount(newPage);
	}
	else{
		//insert in old page
		int keylen = 0;
		int offset = 0;
		getOffset(oldPage, key, keylen, type, offset);
		shiftRightAndInsertEntry(oldPage, entry, entrySize, offset);

		decrementFreeSpace(oldPage, entrySize);
		incrementEntryCount(oldPage);
	}


//	printLeafPageEntries(oldPage, type);
//	printLeafPageEntries(newPage, type);


//	printMyLeafEntries(oldPage, type);
//	printMyLeafEntries(newPage, type);

	//write to the disk
	ixfileHandle.fileHandle.appendPage(newPage);

	short oldPageNum = 0;
	getOwnPageNumber(oldPage, oldPageNum);
	ixfileHandle.fileHandle.writePage(oldPageNum, oldPage);


	return 0;
}

RC getOffsetOfLastEntry(void* page, int &offsetOfLastEntry, AttrType type){

	short entryCount = 0;
	getEntryCount(page, entryCount);

	offsetOfLastEntry = METADATA;

	void* key = malloc(PAGE_SIZE);
	int keyLength = 0;
	extractKey(page, offsetOfLastEntry, type, key, keyLength);

	//search for the correct position
	for(int i=0; i<entryCount; i++){
		offsetOfLastEntry += keyLength + sizeof(short);
	}

	return 0;
}


RC insertRec(IXFileHandle &ixfileHandle, short pageNum, const void* key, const RID rid, void* pushKey, short &pushVal, AttrType type){

int incoming_key;
	memcpy(&incoming_key, key, 4);
	if(incoming_key == 500){
		int y=1;
	}

	void* page = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(pageNum, page);

	short value = 0;
	getLeafFlag(page, value);

	bool isLeaf = (value == LEAF_FLAG) ? true : false;

	// non-leaf
	if(!isLeaf){
		int offsetToBeRead = 0;

		int keyLength = 0;
		getOffset(page, key, keyLength, type, offsetToBeRead);

		//get no of entries in the non leaf page
		short entries = 0;
		getEntryCount(page, entries);

		int offsetOfLastEntry = 0;
		getOffsetOfLastEntry(page, offsetOfLastEntry, type);

		void* nonLeafKey = malloc(PAGE_SIZE);
		int nonLeafKeyLength = 0;

		short pgNum = 0;

		if(offsetToBeRead >= offsetOfLastEntry){
			//assuming always equal to last offset
			memcpy(&pgNum, (char*)page+offsetToBeRead-sizeof(short), sizeof(short));
		}
		else{
			extractKey(page, offsetToBeRead, type, nonLeafKey, nonLeafKeyLength);

			if(compareKey(key, nonLeafKey, type) < 0){
				if(offsetToBeRead == METADATA){
					getLeftPageNum(page, pgNum);
				}
				else{
					memcpy(&pgNum, (char*)page+offsetToBeRead-sizeof(short), sizeof(short));
				}
			}
			else{
				memcpy(&pgNum, (char*)page+offsetToBeRead+keyLength, sizeof(short));
			}

		}
		//call recursively
		insertRec(ixfileHandle, pgNum, key, rid, pushKey, pushVal, type);




		//calc freespace
		short freeSpace = 0;
		getFreeSpace(page, freeSpace);

		if(pushVal == -1){
			//no split
			return 0;
		}
		else{
			//split at lower lvl

			//prepare leaf entry
			void* entry = malloc(PAGE_SIZE);
			short entrySize = 0;
			prepareNonLeafEntry(pushVal, pushKey, entry, type, entrySize);


			if(freeSpace >= entrySize){
				//normal insert
				int pklen = 0;
				int pkoffset = 0;
				getOffset(page, pushKey, pklen, type, pkoffset);

				writeIntoPageBuffer(page, entry, entrySize, pkoffset);

				short pg = 0;
				getOwnPageNumber(page, pg);
				writeIntoPage(ixfileHandle, page, pg);

				pushVal = -1;
			}
			else{
				//split
				splitNonLeaf(ixfileHandle, page, key, type, entry, entrySize, pushKey, pushVal);
				return 0;
			}
		}

	}
	//leaf
	else{
		short freeSpace = 0;
		getFreeSpace(page, freeSpace);

		//prepare leaf entry
		void* entry = malloc(PAGE_SIZE);
		short entrySize = 0;
		prepareLeafEntry(key, entry, type, rid, entrySize);

		//freespace >= key+RID to be inserted
		if(freeSpace >= entrySize){
			//offsetToBeInserted shld be logically 0 for getOffset function
			int offsetToBeInserted = 0;

			int keyLength = 0;
			getOffset(page, key, keyLength, type, offsetToBeInserted);

			writeIntoPageBuffer(page, entry, entrySize, offsetToBeInserted);

			//write into disk
			writeIntoPage(ixfileHandle, page, pageNum);

			//set pushkey to -1, implying no split
//			int pushkey = -1;
//			memcpy(pushKey, &pushkey, sizeof(int));
			pushVal = -1;
		}
		//no freespace - page full
		else{
			//call split
			splitLeaf(ixfileHandle, page, key, type, entry, entrySize, pushKey, pushVal);
		}
	}

	return 0;
}

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	short rootPageNum = (short)ixfileHandle.fileHandle.getCounter(5);

	// First entry ever
	if(rootPageNum == NO_ROOT){
		// Create your page meta data - till now 30 bytes
		void* page_data = malloc(PAGE_SIZE);
		initializePage(page_data);

//		void* page_data = malloc(PAGE_SIZE);
//		ixfileHandle.fileHandle.readPage(0, page_data);

		//prepare leaf entry
		void* entry = malloc(PAGE_SIZE);
		short entrySize = 0;

		prepareLeafEntry(key, entry, attribute.type, rid, entrySize);

		//offsetToBeInserted shld be logically 0 for getOffset function
		int offsetToBeInserted = METADATA;

		//insertion sort
		int keyLength = 0;
		getOffset(page_data, key, keyLength, attribute.type, offsetToBeInserted);
		//cout << "Returned offset to be inserted : " << offsetToBeInserted << endl;

		writeIntoPageBuffer(page_data, entry, entrySize, offsetToBeInserted);

		//
		short pageNumber = (short)(ixfileHandle.fileHandle.getNumberOfPages());
		setOwnPageNumber(page_data, pageNumber);

		ixfileHandle.fileHandle.appendPage(page_data);

		//printPage
//		cout<<"Page no."<<pageNumber<<endl;
//		cout<<"Page content."<<pageNumber<<endl;
//		void* pageContent = malloc(PAGE_SIZE);
//		for(int i=0; i<1; i++){
//			memcpy()
//			cout<<
//
//		}
//		cout<<"------"<<endl<<endl;
		//updating the root page number
		ixfileHandle.fileHandle.setCounter(5, pageNumber);

	}
	else{
		//traverse logic to the desired location
		void* pushKey = malloc(PAGE_SIZE);
		short pushVal = -1;

		//call recursive function
		insertRec(ixfileHandle, rootPageNum, key, rid, pushKey, pushVal, attribute.type);

		//if pushVal == -1 => no split
		//else root split occured create a new root and update the hidden page for the new root page
		if(pushVal != -1){
			//root creation
			void* entry = malloc(PAGE_SIZE);
			short entrySize = 0;
			prepareNonLeafEntry(pushVal, pushKey, entry, attribute.type, entrySize);

			void* rootPage = malloc(PAGE_SIZE);
			initializePage(rootPage);

			writeIntoPageBuffer(rootPage, entry, entrySize, METADATA);

			setLeafFlag(rootPage, DEFAULT_VAL);

			setLeftPageNum(rootPage, rootPageNum);

			short newRootPgNum = (short)ixfileHandle.fileHandle.getNumberOfPages();

			setOwnPageNumber(rootPage, newRootPgNum);


			//printMyLeafEntries(rootPage, attribute.type);

			ixfileHandle.fileHandle.appendPage(rootPage);

			//updating the root page number
			ixfileHandle.fileHandle.setCounter(5, newRootPgNum);
		}

	}
//
//	void* page = malloc(PAGE_SIZE);
//
//	ixfileHandle.fileHandle.readPage(0, page);
//
//	int temp = 0;
//	memcpy(&temp, (char*)page+METADATA, sizeof(int));
//	cout<<"---KEY---"<<temp<<endl;
//

//	//checking all the page data
//	int pg = (int)ixfileHandle.fileHandle.getNumberOfPages();
//	short* linkedList = (short*)malloc(pg*2);
//	void* testData = malloc(PAGE_SIZE);
//	for(int i=0; i<pg; i++){
//		ixfileHandle.fileHandle.readPage(i, testData);
//		getPointerToRight(testData, linkedList[i]);
//		printMyPageEntries(testData, attribute.type);
//	}
//
//	for(int i=0; i<pg; i++){
//		cout<<"Pg :"<<i<<" nextPtr: "<<linkedList[i]<<endl;
//	}

    return 0;
}

RC getPosition(void* page, const void* nextkey, bool lowKeyInclusive, AttrType type, int &position){

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	position = METADATA;

	int found = -1;

	void* key = malloc(PAGE_SIZE);
	int keyLength = 0;
	//search for the correct position
	for(int i=0; i<entryCount; i++){
		extractKey(page, position, type, key, keyLength);

		if(lowKeyInclusive){
			if(compareKey(key, nextkey, type) >= 0){
				found = 0;
				break;
			}
		}
		else{
			if(compareKey(key, nextkey, type) > 0){
				found = 0;
				break;
			}
		}

		position += keyLength + sizeof(RID);

	}

	return found;
}

RC getPositionWithCount(void* page, const void* nextkey, bool lowKeyInclusive, AttrType type, int &position, int &visitedkeysCount){

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	position = METADATA;

	int found = -1;

	void* key = malloc(PAGE_SIZE);
	int keyLength = 0;
	//search for the correct position
	for(int i=0; i<entryCount; i++){
		extractKey(page, position, type, key, keyLength);

		if(lowKeyInclusive){
			if(compareKey(key, nextkey, type) >= 0){
				found = 0;
				break;
			}
		}
		else{
			if(compareKey(key, nextkey, type) > 0){
				found = 0;
				break;
			}
		}

		position += keyLength + sizeof(RID);
		visitedkeysCount++;
	}

	return found;
}

RC getRightNonEmptyPage(IXFileHandle &ixfileHandle, short &pagenum){

	void* page = malloc(PAGE_SIZE);
	short entries = 0;
	while(true){
			ixfileHandle.fileHandle.readPage(pagenum, page);

			// getEntryCount
			getEntryCount(page, entries);

			// Deleted page, assign pagenum as rightpagenum
			if(entries == 0){
				getPointerToRight(page, pagenum);
			}
			else{
				break;
			}
	}

	return 0;
}

short traverse(IXFileHandle &ixfileHandle, const void* searchKey, bool searchKeyInclusive, AttrType type, const short rootPageNum){

	short pageNum = 0;

	//if ROOT not created
	if(rootPageNum == 0){
		return rootPageNum;
	}
	//else ROOT created
	else{
		void* page = malloc(PAGE_SIZE);

		ixfileHandle.fileHandle.readPage(rootPageNum, page);

		short value = 0;
		getLeafFlag(page, value);

		bool isLeaf = (value == LEAF_FLAG) ? true : false;

		int keyLength = 0;
		int offset = 0;

		void* extractedKey = malloc(PAGE_SIZE);
		int extractedKeyLength = 0;

		while(!isLeaf){

			getOffset(page, searchKey, keyLength, type, offset);

			extractKey(page, offset, type, extractedKey, extractedKeyLength);

			if(compareKey(searchKey, extractedKey, type) < 0){
				//left
				if(offset == METADATA){
					getLeftPageNum(page, pageNum);
				}
				else{
					memcpy(&pageNum, (char*)page+offset-sizeof(short), sizeof(short));
				}

			}
			else{
				//right
				memcpy(&pageNum, (char*)page+offset+extractedKeyLength, sizeof(short));
			}

			ixfileHandle.fileHandle.readPage(pageNum, page);

			getLeafFlag(page, value);

			isLeaf = (value == LEAF_FLAG) ? true : false;
		}

		//tht condition
		int position = 0;
		int visitedKeyCount = 0;

		getPositionWithCount(page, searchKey, searchKeyInclusive, type, position, visitedKeyCount);

		short entries = 0;
		getEntryCount(page, entries);

		if(visitedKeyCount == entries){
			getPointerToRight(page, pageNum);
			getRightNonEmptyPage(ixfileHandle, pageNum);
		}

	}

	return pageNum;
}

RC shiftLeft(void* page, const int offset, const int shiftLength){
	void* buffer = malloc(PAGE_SIZE);

	memcpy(buffer, page, offset);
	memcpy((char*)buffer+offset, (char*)page+offset+shiftLength, PAGE_SIZE-offset-shiftLength);

	memcpy(page, buffer, PAGE_SIZE);

	free(buffer);
	return 0;
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{

	//fetch root page num
	int rootPageNum = ixfileHandle.fileHandle.getCounter(5);

	//cout<<"RootPagenumber : "<<rootPageNum<<endl;

	//read after traversing the tree
	void* page = malloc(PAGE_SIZE);

	short pageNum = traverse(ixfileHandle, key, true, attribute.type, rootPageNum);

	ixfileHandle.fileHandle.readPage(pageNum, page);

	//getOffset()
	int offset = 0;
	int keyLength = 0;
	getOffset(page, key, keyLength, attribute.type, offset);


	//move to the key and compare the rid then delete
	void* entryKey = malloc(PAGE_SIZE);
	int entryKeyLength = 0;

	RID entryRid;

	while(true){
		extractKey(page, offset, attribute.type, entryKey, entryKeyLength);

		if(memcmp(entryKey, key, entryKeyLength) == 0){
			memcpy(&entryRid, (char*)page+offset+entryKeyLength, sizeof(RID));
			if(entryRid.pageNum == rid.pageNum && entryRid.slotNum==rid.slotNum){
				//delete logic
				shiftLeft(page, offset, entryKeyLength+sizeof(RID));
				break;
			}
			else{
				offset += entryKeyLength+sizeof(RID);
				if(offset >= PAGE_SIZE){
					return -1;
				}
			}
		}
		else{
			return -1;
		}
	}

	//update metadata

	//decrement Entry Count
	decrementEntryCount(page);

	//Check if page has entries after deletion
	//if not update the deleted flag
	short noOfEntries = 0;
	getEntryCount(page, noOfEntries);
	if(noOfEntries == 0){
		//setting the deleted flag for the page
		setDeletedFlag(page, 1);
	}

	//update freespace
	incrementFreeSpace(page, entryKeyLength+sizeof(RID));

	//write the changes to the page
	writeIntoPage(ixfileHandle, page, pageNum);

    return 0;
}

RC searchLeftmostLeafNode(IXFileHandle &ixfileHandle, void *lowKey, AttrType type, short &pgNum){

	void* page = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(0, page);

	short entries = 0;
	getEntryCount(page, entries);



	if(entries == 0){
		getRightNonEmptyPage(ixfileHandle, pgNum);
		ixfileHandle.fileHandle.readPage(pgNum, page);
		cerr << "No enries.... moving to right non-empty sibling"<<endl;
	}

	int lowKeyLen = 0;
	extractKey(page, METADATA, type, lowKey, lowKeyLen);

	return 0;
}

int getlastEntryPosition(void* page){
	int offset = METADATA;

	short entryCount = 0;
	getEntryCount(page, entryCount);

	int varlength = 0;
	for(int i=0; i<entryCount-1; i++){
		memcpy(&varlength, (char*)page + offset, sizeof(int));
		offset = (sizeof(int) + varlength) + sizeof(RID);
	}
	return offset;
}

RC searchRightmostLeafNode(IXFileHandle &ixfileHandle, void *highKey, AttrType type){

	short rootPageNum = (short)ixfileHandle.fileHandle.getCounter(5);

	if(rootPageNum == 0){
		if(ixfileHandle.fileHandle.getNumberOfPages() == 0){
			return -1;
		}
		else{
			//get entryCount
			void* page = malloc(PAGE_SIZE);
			ixfileHandle.fileHandle.readPage(0, page);

			short entryCount = 0;
			getEntryCount(page, entryCount);

			int offset = METADATA;

			switch(type){
				//handling int and real
				case TypeInt:{
					offset += (entryCount-1)*(sizeof(int)+sizeof(RID));
					//highKey = malloc(sizeof(int));
					memcpy(highKey, (char*)page + offset, sizeof(int));
					break;
				}

				case TypeReal:{
					offset += (entryCount-1)*(sizeof(int)+sizeof(RID));
					//highKey = malloc(sizeof(int));
					memcpy(highKey, (char*)page+offset, sizeof(float));
					break;
				}
				//handling varchar
				case TypeVarChar:{
					int Varlength = 0;
					int lastEntryoffset = getlastEntryPosition(page);

					int varlength = 0;
					memcpy(&varlength, (char*)page+lastEntryoffset, sizeof(int));

					//highKey = malloc(sizeof(int)+varlength);
					memcpy(highKey, (char*)page+lastEntryoffset, sizeof(int) + Varlength);

					break;
				}
			}
		}
	}
	else{
		//TODO: call traverse()

	}

	return 0;
}


RC IndexManager::scan(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        const void      *highKey,
        bool			lowKeyInclusive,
        bool        	highKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
	if(ixfileHandle.fileHandle.handle == NULL){
		return -1;
	}

	int rootPg = ixfileHandle.fileHandle.getCounter(5);

	// if index is empty
	if(rootPg == NO_ROOT){
		return -1;
	}

	ix_ScanIterator.ixfileHandle = &ixfileHandle;
	ix_ScanIterator.attribute = &attribute;

	// Never free these mallocs, this should be free in close of the iterator.
	ix_ScanIterator.lowKey = malloc(PAGE_SIZE);
	ix_ScanIterator.nextKey = malloc(PAGE_SIZE);

	int lowKeylength = 0;
	int highKeylength = 0;
	int nextKeyLen = 0;

	if(lowKey){
		// lowKey is not NULL
		extractKey(lowKey, 0, attribute.type, ix_ScanIterator.lowKey, lowKeylength);
		ix_ScanIterator.lowKeyInclusive = lowKeyInclusive;

		ix_ScanIterator.nextKeyPageNum = traverse(ixfileHandle, lowKey, lowKeyInclusive, attribute.type, rootPg);
	}
	else{
		// lowKey is NULL
		// Setting low key as leftmost key in leftmost leaf page with inclusiveness as true
		ix_ScanIterator.lowKeyInclusive = true;
		if(searchLeftmostLeafNode(ixfileHandle, ix_ScanIterator.lowKey, attribute.type, ix_ScanIterator.nextKeyPageNum)==-1){
			return -1;
		}
	}

	if(highKey){
		// highKey is not NULL

		ix_ScanIterator.highKey = malloc(PAGE_SIZE);
		extractKey(highKey, 0, attribute.type, ix_ScanIterator.highKey, highKeylength);
		ix_ScanIterator.highKeyInclusive = highKeyInclusive;
	}
	else{
		// highKey is NULL
		// Deal this NULL case in getNextRecord
		ix_ScanIterator.highKey = NULL;
		ix_ScanIterator.highKeyInclusive = true;
	}

	//TODO: if page 0 is deleted page, but index is not empty.

	// Setting nextKey as the lowKey for the first iteration
	extractKey(ix_ScanIterator.lowKey, 0, attribute.type, ix_ScanIterator.nextKey, nextKeyLen);

	return 0;

}

RC printNonLeafPageEntries(void* page, AttrType type, const int entryCount){
	switch(type){
		case TypeInt:{

			break;
		}

		case TypeReal:{

			break;
		}

		case TypeVarChar:{

			break;
		}
	}

	return 0;
}

RC printLeafPageEntries(void* page, const AttrType type, const int entryCount){
	int offset = METADATA;

	switch(type){
		case TypeInt:{

			//leaf - 12 bytes
			// key + rid
			//int leafPageEntrySize = sizeof(int) + (sizeof(RID));

			int key = 0;

			int prevKey  = -1;

			//search for the correct position
			for(int i=0; i<entryCount; i++){

				//read key of entry 'i'
				memcpy(&key, (char*)page+offset, sizeof(int));
				offset += sizeof(int);

				//read rid of entry 'i'
				RID rid;
				memcpy(&rid, (char*)page+offset, sizeof(RID));
				offset += sizeof(RID);

				if(key == prevKey){
					cout << "(" << rid.pageNum << "," << rid.slotNum << ")";
				}
				else{
					if(i!=0){
						cout<<"]\",";
					}
					cout << "\"" << key << ":";

					cout << "[(" << rid.pageNum << "," << rid.slotNum << ")";
					prevKey = key;
				}


				if(i!=entryCount-1){
					cout << ",";
				}
				if(i==entryCount-1){
					cout << "]\"";
				}
			}

			break;
		}

		case TypeReal:{

			break;
		}

		case TypeVarChar:{

			break;
		}
	}


	return 0;
}

RC printPageEntries(void* page, AttrType type){
	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	short leaf = 0;
	getLeafFlag(page, leaf);


	bool isLeaf = (leaf==LEAF_FLAG) ? true : false;
	//cout<< "IS leaf : " << isLeaf<<endl;

	if(isLeaf){
		printLeafPageEntries(page, type, entryCount);
	}
	else{
		printNonLeafPageEntries(page, type, entryCount);
	}

	return 0;

}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {

	short root_page_num = (short)ixfileHandle.fileHandle.getCounter(5);

	if(root_page_num == NO_ROOT){

		//check number of pages
		short numOfPages = (short)ixfileHandle.fileHandle.getNumberOfPages();

		if(numOfPages == 0){
			cout << "{\n}" << endl;
		}
		else if(numOfPages == 1){
			cout << "{" << endl;

			//read page
			void* pageData = malloc(PAGE_SIZE);
			ixfileHandle.fileHandle.readPage(0, pageData);

			short entryCount = 0;
			getEntryCount(pageData, entryCount);

			cout << "\"keys\": [";

			printPageEntries(pageData, attribute.type);

			cout << "\"]" << endl;
			//"keys": ["Q:[(10,1)]","R:[(11,1)]","S:[(12,1)]"]}

			cout << "}" << endl;
		}


	}
	else{

	}

}

IX_ScanIterator::IX_ScanIterator()
{
//	lowKey = NULL;
//	highKey = NULL;
	lowKeyInclusive = false;
	highKeyInclusive = false;
	nextKey = NULL;
	nextKeyPageNum = 0;
}

IX_ScanIterator::~IX_ScanIterator()
{
}




RC IX_ScanIterator::findHit(RID &rid, void* key, AttrType type){

	// no other matching record exist in the file, end of file reached
	if(nextKeyPageNum == -1){
		return -1;
	}
//	// Read rootPageNum
//	short rootPageNum = (short)ixfileHandle->fileHandle.getCounter(5);

	// Traversing to reach the desired leaf page
//	nextKeyPageNum = traverse(*ixfileHandle, nextKey, true, type, rootPageNum);

	// Creating variables for later use
	void* page = malloc(PAGE_SIZE);
	//reading next key page, look for hit in this page only
	ixfileHandle->fileHandle.readPage(nextKeyPageNum, page);

	bool hit_flag = false;
	int nextKeyLength = 0;
	void* rightPage = malloc(PAGE_SIZE);
	short rightPageNum = 0;

	// -get the position of nextKey in the page on the basis of inclusiveness boolean of lowKey
	int position = 0;
	//-getHitPosition
	if(compareKey(nextKey, lowKey, type) == 0){
		// first time comparison if nextKey == lowKey, check for inclusiveness too
		getPosition(page, nextKey, lowKeyInclusive, type, position);
	}
	else{
		// No need to check for inclusivity of the lower bound, since not first iteration
		int tempLen = 0;
		getOffset(page, nextKey, tempLen, type, position);
	}

	// Got the position, now check where position is less than PAGE_SIZE or not
	if(position <= PAGE_SIZE){

		// FoundHit
		extractKey(page, position, type, key, keyLength);

		if(highKey == NULL){
			// if highKey is null
			memcpy(&rid, (char*)page + position + keyLength, sizeof(RID));
			hit_flag = true;
		}
		else{
			// highKey is not null
			//Now just compare the hitkey with upper bound
			if(highKeyInclusive){
				// upper bound inclusive
				if(compareKey(key, highKey, type) <= 0){
					//return 'hit' RIDs and key
					memcpy(&rid, (char*)page + position + keyLength, sizeof(RID));
					hit_flag = true;
				}
			}
			else{
				// upper bound exclusive
				if(compareKey(key, highKey, type) < 0){
					//return 'hit' RIDs and key
					memcpy(&rid, (char*)page + position + keyLength, sizeof(RID));
					hit_flag = true;
				}
			}
		}

		if(hit_flag == false){
			// no hit
			return -1;
		}

		//-Now, set nextKey here for next call, don't return anything here at all
		//nextKeyPosition
		int nextposition = position + keyLength + sizeof(RID);


		if(nextposition >= PAGE_SIZE){
			//go to right sibling page


			getPointerToRight(page, rightPageNum);

			//reached end of the B+ tree
			if(rightPageNum == -1){
				// it will return -1 directly in the next iteration of findHit
				nextKeyPageNum = -1;
			}
			else{
				// right page exists
				// read the first key of this page and set it as nextKey
				ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);

				// if right page is deleted page, go to further right sibling
				short rightPgEntries = 0;
				getEntryCount(rightPage, rightPgEntries);

				if(rightPgEntries == 0){
					getRightNonEmptyPage(*ixfileHandle, rightPageNum);
					ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);
				}


				extractKey(rightPage, METADATA, type, nextKey, nextKeyLength);
				nextKeyPageNum = rightPageNum;
			}
		}
		else{

			// extract nextKey just after the hitKey

			// what if this was the last key, toh extractkey fat jayega.....
			short freespacethispage = 0;
			getFreeSpace(page, freespacethispage);

			if(nextposition >= (PAGE_SIZE - freespacethispage)){

				getPointerToRight(page, rightPageNum);

				// Small addition
				if(rightPageNum == -1){
					nextKeyPageNum = -1;
					return 0;
				}

				ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);

				// if right page is deleted page, go to further right sibling
				short rightPgEntries = 0;
				getEntryCount(rightPage, rightPgEntries);

				if(rightPgEntries == 0){
					getRightNonEmptyPage(*ixfileHandle, rightPageNum);
					ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);
				}


				extractKey(rightPage, METADATA, type, nextKey, nextKeyLength);
				nextKeyPageNum = rightPageNum;
//
			}

			// bindaas extract karo
			extractKey(page, nextposition, type, nextKey, nextKeyLength);
		}

	}
	else{
		// no hit in this page
		// No corner case, if the matching record is not in this page
		// That means, end of file reached.
		return -1;
	}

	// Hit found
	return 0;
}



RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	if(findHit(rid, key, attribute->type) == -1){
		return -1;
	}

	return 0;
}

RC IX_ScanIterator::close()
{

	//TODO: complete this function

	return 0;

}

IXFileHandle::IXFileHandle()
{
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
}

IXFileHandle::~IXFileHandle()
{
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	if(fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount) != -1)
		return 0;
    return -1;
}

