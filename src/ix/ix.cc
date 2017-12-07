
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
RC prepareLeafEntry(const void* indexKey, void* indexEntry, AttrType type, const RID &rid, short &entrySize){

	switch(type){
		case TypeInt:{

			int offset = 0;
			memcpy((char*)indexEntry+offset, indexKey, sizeof(int));
			offset += sizeof(int);

			memcpy((char*)indexEntry+offset, &rid, sizeof(RID));
			offset += sizeof(RID);

			entrySize = offset;
			break;
		}
		case TypeReal:{

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

			//key
			int offset = 0;
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
			// extract only the varcharsize
			memcpy(&keyLength, (char*)page+offset, sizeof(int));
			// set whole key
			memcpy(key, (char*)page+offset, sizeof(int)+keyLength);
			// set keylength as [sizeof(int) + varcharsize]
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
			int keyLength1 = 0;
			memcpy(&keyLength1, key1, sizeof(int));

			char* varcharkey1 = (char*) malloc(keyLength1);
			memcpy(varcharkey1, (char*)key1 + sizeof(int), keyLength1);
			//varcharkey1[keyLength1] = '\0';

			string varkey1(varcharkey1, keyLength1);

			int keyLength2 = 0;
			memcpy(&keyLength2, key2, sizeof(int));
			char* varcharkey2 = (char*) malloc(keyLength2);
			memcpy(varcharkey2, (char*)key2 + sizeof(int), keyLength2);
			//varcharkey2[keyLength2] = '\0';

			string varkey2(varcharkey2, keyLength2);

			status = varkey1.compare(varkey2);

			//status = strcmp(varcharkey1, varcharkey2);

			free(varcharkey1);
			free(varcharkey2);
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

	free(key);
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

	shiftRightAndInsertEntry(page, entry, entrySize, offsetToBeInserted);

	// update number of entries
	incrementEntryCount(page);

	// update freespace
	decrementFreeSpace(page, entrySize);

	return 0;
}

//called iff there is free space
RC writeIntoPage(IXFileHandle &ixfileHandle, void* page_data, short pageNumber){
	ixfileHandle.fileHandle.writePage(pageNumber, page_data);
	return 0;
}

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

	free(prevKey);

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
	free(buffer);

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

	if(newPage){
		free(newPage);
		newPage = NULL;
	}

	return 0;
}

RC splitLeaf(IXFileHandle &ixfileHandle, void* oldPage, const void* key, AttrType type, const void* entry, const int entrySize, void* pushKey, short &pushVal){

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
	free(buffer);

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

	//write to the disk
	ixfileHandle.fileHandle.appendPage(newPage);

	short oldPageNum = 0;
	getOwnPageNumber(oldPage, oldPageNum);
	ixfileHandle.fileHandle.writePage(oldPageNum, oldPage);

	if(newPage){
		free(newPage);
		newPage = NULL;
	}

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

	free(key);
	return 0;
}

RC insertRec(IXFileHandle &ixfileHandle, short pageNum, const void* key, const RID rid, void* pushKey, short &pushVal, AttrType type){

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
			if(page){
				free(page);
				page = NULL;
			}

			if(nonLeafKey){
				free(nonLeafKey);
				nonLeafKey = NULL;
			}

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

				if(page){
						free(page);
						page = NULL;
				}

				if(nonLeafKey){
					free(nonLeafKey);
					nonLeafKey = NULL;
				}

				if(entry){
					free(entry);
					entry = NULL;
				}

				return 0;
			}

			if(entry){
				free(entry);
				entry = NULL;
			}
		}

		if(nonLeafKey){
			free(nonLeafKey);
			nonLeafKey = NULL;
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

			pushVal = -1;
		}
		//no freespace - page full
		else{
			//call split
			splitLeaf(ixfileHandle, page, key, type, entry, entrySize, pushKey, pushVal);
		}

		if(entry){
			free(entry);
			entry = NULL;
		}
	}

	if(page){
		free(page);
		page = NULL;
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

		//updating the root page number
		ixfileHandle.fileHandle.setCounter(5, pageNumber);

		free(page_data);
		free(entry);
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


			ixfileHandle.fileHandle.appendPage(rootPage);

			//updating the root page number
			ixfileHandle.fileHandle.setCounter(5, newRootPgNum);

			free(entry);
			free(rootPage);
		}

		if(pushKey){
			free(pushKey);
			pushKey = NULL;
		}

	}

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

	free(key);
	return found;
}

RC getPositionWithCount(void* page, const void* nextkey, bool lowKeyInclusive, AttrType type, int &position, int &visitedkeysCount){

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	short leaf = 0;
	getLeafFlag(page, leaf);
	bool is_leaf = (leaf == LEAF_FLAG) ? true : false;

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

		if(is_leaf){
			// key + rid
			position += keyLength + sizeof(RID);
		}
		else{
			// key + rightchild
			position += keyLength + sizeof(short);
		}

		visitedkeysCount++;
	}

	free(key);
	return found;
}

RC getRightNonEmptyPage(IXFileHandle &ixfileHandle, short &pagenum){

	void* page = malloc(PAGE_SIZE);
	short entries = 0;
	while(true){
			// trying to read an invalid page
			if(ixfileHandle.fileHandle.readPage(pagenum, page) == -1){
				free(page);
				return -1;
			}
			else{
				// decrease this readPageCount
				// read mode - 1, decrease readPageounter by 1.
				ixfileHandle.fileHandle.decreaseCounter(1);
			}

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

	free(page);
	return 0;
}

RC getRightNonEmptyPageWithPageBuffer(IXFileHandle &ixfileHandle, short &pagenum, void *page){

	short entries = 0;

	while(true){

			if(ixfileHandle.fileHandle.readPage(pagenum, page) == -1)
				return -1;

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

	//if ROOT == leaf page
	if(rootPageNum == 0){
		// checking special condition where index is empty
		void* page = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(rootPageNum, page);
		short entries = 0;
		getEntryCount(page, entries);

		// Very very rare condition
		if(entries == 0){
			// read mode - 1, decrease readPageounter by 1.
			ixfileHandle.fileHandle.decreaseCounter(1);

			free(page);
			return -1;
		}

		ixfileHandle.fileHandle.decreaseCounter(1);
		free(page);

		// returning the only page in the file
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
			// for non-leaf

			//add logic to chk if it is last entry
			getOffset(page, searchKey, keyLength, type, offset);

			int position = 0;
			int visitedKeyCount = 0;

			getPositionWithCount(page, searchKey, searchKeyInclusive, type, position, visitedKeyCount);

			short entries = 0;
			getEntryCount(page, entries);

			// special condition
			// [offset is at the end of page, drag it back with one key+right block]
			if(visitedKeyCount == entries){
				offset = offset - keyLength - sizeof(short);
			}

			extractKey(page, offset, type, extractedKey, extractedKeyLength);

			if(compareKey(searchKey, extractedKey, type) < 0){
				//left child page num (<)
				if(offset == METADATA){
					getLeftPageNum(page, pageNum);
				}
				else{
					memcpy(&pageNum, (char*)page+offset-sizeof(short), sizeof(short));
				}

			}
			else{
				//right child page num (>=)
				memcpy(&pageNum, (char*)page+offset+extractedKeyLength, sizeof(short));
			}

			ixfileHandle.fileHandle.readPage(pageNum, page);

			getLeafFlag(page, value);

			isLeaf = (value == LEAF_FLAG) ? true : false;
		}

		//that condition [reached leaf, but it might not be present in this page].
		int position = 0;
		int visitedKeyCount = 0;

		getPositionWithCount(page, searchKey, searchKeyInclusive, type, position, visitedKeyCount);

		short entries = 0;
		getEntryCount(page, entries);

		if(visitedKeyCount == entries){
			void* buffer = malloc(PAGE_SIZE);
			getPointerToRight(page, pageNum);
			// [check for non-empty page.]
			getRightNonEmptyPageWithPageBuffer(ixfileHandle, pageNum, buffer);
			free(buffer);
		}

		free(page);
		free(extractedKey);
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

	short pageNum = traverse(ixfileHandle, key, true, attribute.type, rootPageNum);

	//read after traversing the tree
	void* page = malloc(PAGE_SIZE);
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

		// compare entryKey and key for equality
		if(compareKey(entryKey, key, attribute.type) == 0){
			// keys are equal, now check for rid's
			memcpy(&entryRid, (char*)page+offset+entryKeyLength, sizeof(RID));
			if(entryRid.pageNum == rid.pageNum && entryRid.slotNum==rid.slotNum){
				//delete logic
				shiftLeft(page, offset, entryKeyLength+sizeof(RID));
				break;
			}
			else{
				offset += entryKeyLength+sizeof(RID);
				if(offset >= PAGE_SIZE){
					free(page);
					free(entryKey);
					return -1;
				}
			}
		}
		else{
			free(page);
			free(entryKey);
			return -1;
		}
	}

	//update metadata [entryCount, freespace]

	//decrement Entry Count
	decrementEntryCount(page);

	//update freespace
	incrementFreeSpace(page, entryKeyLength+sizeof(RID));

	// Setting the deleted flag, special condition
	// actually no need for this, we are not even using the deleted flag anywhere.
	short noOfEntries = 0;
	getEntryCount(page, noOfEntries);
	if(noOfEntries == 0){
		//setting the deleted flag for the page
		setDeletedFlag(page, 1);
	}

	//write the changes to the page
	writeIntoPage(ixfileHandle, page, pageNum);

	free(page);
	free(entryKey);
    return 0;
}

RC searchLeftmostLeafNode(IXFileHandle &ixfileHandle, void *lowKey, AttrType type, short &pgNum){

	pgNum = 0;

	void* page = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(0, page);

	// read mode - 1, decrease readPageounter by 1.
	//ixfileHandle.fileHandle.decreaseCounter(1);

	short entries = 0;
	getEntryCount(page, entries);

	if(entries == 0){
		getRightNonEmptyPage(ixfileHandle, pgNum);
		if(ixfileHandle.fileHandle.readPage(pgNum, page)==-1){
			pgNum = -1;
			free(page);
			return -1;
		}else{
			// read mode - 1, decrease readPageounter by 1.
			ixfileHandle.fileHandle.decreaseCounter(1);
		}
	}

	int lowKeyLen = 0;
	extractKey(page, METADATA, type, lowKey, lowKeyLen);

	free(page);
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

	// No index on disk, return -1
	if(rootPg == NO_ROOT){
		return -1;
	}

	ix_ScanIterator.ixfileHandle = ixfileHandle;
	ix_ScanIterator.attribute = attribute;

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

		// if index is empty, it will be -1, otherwise the value of the desired page number.
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

	// Setting nextKey as the lowKey for the first iteration
	extractKey(ix_ScanIterator.lowKey, 0, attribute.type, ix_ScanIterator.nextKey, nextKeyLen);

	ix_ScanIterator.nextKeyBuffer = malloc(PAGE_SIZE);
	return 0;

}

RC printKey(void* key, AttrType type){
	switch(type){
		case TypeInt:{
			int keyvalue = 0;
			memcpy(&keyvalue, key, sizeof(int));
			cout << keyvalue;
			break;
		}
		case TypeReal:{
			float keyvalue = 0.0;
			memcpy(&keyvalue, key, sizeof(float));
			cout << keyvalue;
			break;
		}
		case TypeVarChar:{
			// extract keylength first
			int keylen = 0;
			memcpy(&keylen, key, sizeof(int));

			char* keyvalue = (char*) malloc(keylen);

			// extract only keyVarChar
			memcpy(keyvalue, (char*) key + sizeof(int), keylen);

			// put that into string
			string mykey((char*)keyvalue, keylen);

			// print that string
			cout << mykey;

			free(keyvalue);
			break;
		}
	}

	return 0;
}

RC printNonLeafPageEntries(void* page_data, AttrType type){

	// First key starts from this position
	int offset = METADATA;

	short entry_count = 0;
	getEntryCount(page_data, entry_count);

	cout<<"\"keys\":[";

	// Printing all the keys only, Non-Leaf page, no duplicates handling needed
	void* key = malloc(PAGE_SIZE);
	int keyLength = 0;

	for(int i=0; i<entry_count; i++){
		// extract the key from the desired offset
		extractKey(page_data, offset, type, key, keyLength);
		cout<<"\"";

		// print the key based upon the attribute type
		printKey(key, type);
		cout<<"\"";

		// Just skip the last comma
		if(i != entry_count-1){
			cout<<",";
		}

		// increase the offset such that next key can be extracted in next iteration
		// key + right child
		offset += keyLength + sizeof(short);
	}
	cout<<"]";

	free(key);
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
		if(i!=0 && compareKey(key, prev_key, type) == 0 ){
			dupli_flag = true;
			cout << ",";
		}
		else{
			if(i!=0){
				dupli_flag = false;
				cout << "]\"";
				cout<<",";
			}
		}

		if(dupli_flag == false){
			cout << "\"";
			// print the key based upon the attribute type
			printKey(key, type);
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
		// key + RID
		offset += keyLength + sizeof(RID);

		// Setting prev_key as currentkey for next iteration of duplicate check
		memcpy(prev_key, key, keyLength);
	}

	cout<<"]";

	free(key);
	free(prev_key);
	return 0;
}


RC printPageEntries(void* page, AttrType type, short pageNum){

	short ownpagenum = 0;
	getOwnPageNumber(page, ownpagenum);

	short leaf_flag = 0;
	getLeafFlag(page, leaf_flag);

	bool isLeaf = (leaf_flag == LEAF_FLAG) ? true : false;

	if(isLeaf){
		short entryCount = 0;
		getEntryCount(page, entryCount);

		// empty page/deleted page
		if(entryCount == 0){

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

void printTab(int count){
	for(int i=0; i<count; i++){
		cout << "\t";
	}
}

int myRecursivePrintBTree (IXFileHandle &ixfileHandle, const Attribute &attribute, short root_page_num, int print_count, int visitedkeysCount){

	bool child_flag = false;

	if(root_page_num == -1)
		return -1;

	// All the recursive work here only
	void* page_data = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(root_page_num, page_data);

	short left_page_num = 0;
	short right_page_num = 0;
	short is_leaf = 0;

	getLeafFlag(page_data, is_leaf);
	bool leaf_flag = (is_leaf == (short)LEAF_FLAG) ? true : false;

	// Print Root Node Directly without any spaces
	printPageEntries(page_data, attribute.type, root_page_num);

	if(leaf_flag){
		child_flag = false;
		left_page_num = DEFAULT_VAL;
		right_page_num = DEFAULT_VAL;
	}
	else{
		// NON-LEAF
		child_flag = true;
		short nonleafentries = 0;
		getEntryCount(page_data, nonleafentries);

		void* key = malloc(PAGE_SIZE);
		int keylennn = 0;
		int offset = METADATA;

		// Adjusting spaces if we have any one of the children
		if(child_flag){
			printTab(print_count);
			cout << "," << endl;
			printTab(print_count);
			cout << "\"children\":[" << endl;
		}

		for(int i=0; i<nonleafentries; i++){

			// -extract left_page_num and right_page_num from this page_data

			if(i == 0){
				// First iteration, both left and right

				// set left_page_num for the first time only
				getLeftPageNum(page_data, left_page_num);

				extractKey(page_data, offset, attribute.type, key, keylennn);
				// set right_page_num
				memcpy(&right_page_num, (char*)page_data + offset + keylennn, sizeof(short));

				// key + right child --- increase offset for the next iteration
				offset += keylennn + sizeof(short);

				// call both recursion, left and right child

				// -Recursively call Left Node
				// Adjusting spaces before left child call
				printTab(print_count + 1);
				cout << "{" ;

				// Left child recursive print call
				myRecursivePrintBTree(ixfileHandle, attribute, left_page_num, print_count + 1, visitedkeysCount);
				cout << "}," << endl;

			}
			else{
				// Not the first iteration, only right
				left_page_num = DEFAULT_VAL;

				// set only the right page num
				extractKey(page_data, offset, attribute.type, key, keylennn);
				// set right_page_num
				memcpy(&right_page_num, (char*)page_data + offset + keylennn, sizeof(short));

				// key + right child  --- increase offset for the next iteration
				offset += keylennn + sizeof(short);

				// call only right child recursion
			}


			// -Recursively call Right Node
			// Adjusting spaces before right child call
			printTab(print_count + 1);
			cout << "{" ;

			// Right child recursive print call
			if(right_page_num != DEFAULT_VAL)
				myRecursivePrintBTree(ixfileHandle, attribute, right_page_num, print_count + 1, visitedkeysCount);

			cout << "}";

			if(i != nonleafentries-1){
				cout << "," << endl;
			}

			if(left_page_num != DEFAULT_VAL || right_page_num != DEFAULT_VAL){
				printTab(print_count);
				//cout << "]";
			}

			// increasing print_count for the next iteration
			//print_count ++;

		}// for loop
		cout << endl << "]";


		if(key){
			free(key);
			key = NULL;
		}

	}// Non-leaf

	if(page_data){
		free(page_data);
		page_data = NULL;
	}

	return 0;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {

	cout << "{" << endl;

	short rootpagenum = (short) ixfileHandle.fileHandle.getCounter(5);
	int print_count = 0;

	myRecursivePrintBTree(ixfileHandle, attribute, rootpagenum, print_count, 0);

	cout << endl << "}" << endl;
}

IX_ScanIterator::IX_ScanIterator()
{
	lowKey = NULL;
	nextKey = NULL;
	highKey = NULL;
	nextKeyBuffer = NULL;
	nextKeyBufferFlag = false;
	lowKeyInclusive = false;
	highKeyInclusive = false;
	nextKeyPageNum = 0;
	nextKeyOffset = METADATA;
	iterationCount = 0;
	keyLength = 0;
}

IX_ScanIterator::~IX_ScanIterator()
{
	if(lowKey){
		free(lowKey);
		lowKey = NULL;
	}
	if(nextKey){
		free(nextKey);
		nextKey = NULL;
	}
	if(highKey){
		free(highKey);
		highKey = NULL;
	}
	if(nextKeyBuffer){
		free(nextKeyBuffer);
		nextKeyBuffer = NULL;
	}

}

RC getOffsetWithRID(void* page, const void* searchKey, int &keyLength,const RID &searchKeyRid, AttrType type, int &offsetToBeInserted){

	offsetToBeInserted = METADATA;

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	void* key = malloc(PAGE_SIZE);
	RID rid1;

	//search for the correct position
	for(int i=0; i<entryCount; i++){
		extractKey(page, offsetToBeInserted, type, key, keyLength);
		memcpy(&rid1, (char*)page + offsetToBeInserted + keyLength, sizeof(RID));

		// compare Rid too
		if(compareKey(key, searchKey, type) == 0 && (rid1.pageNum == searchKeyRid.pageNum) && (rid1.slotNum == searchKeyRid.slotNum)){
			break;
		}

		offsetToBeInserted += keyLength + sizeof(RID);
	}

	free(key);
	return 0;
}

RC IX_ScanIterator::findHit(RID &rid, void* key, AttrType type){

	iterationCount ++;

	if(iterationCount == 0){
		nextKeyBufferFlag = false;
	}

	// no other matching record exist in the file, end of file reached
	if(nextKeyPageNum == -1){
		return -1;
	}

	// Creating variables for later use
	void* page = malloc(PAGE_SIZE);

	// my addition of keeping buffer everytime to improve the scan speed
	if(nextKeyBufferFlag == false){
		//reading next key page, look for hit in this page only
		ixfileHandle.fileHandle.readPage(nextKeyPageNum, page);
	}
	else{
		memcpy(page, nextKeyBuffer, PAGE_SIZE);
	}

	bool hit_flag = false;
	int nextKeyLength = 0;
	void* rightPage = malloc(PAGE_SIZE);
	short rightPageNum = 0;

	// -get the position of nextKey in the page on the basis of inclusiveness boolean of lowKey
	int position = 0;
	//-getHitPosition for the first time only
	if(iterationCount == 1){
		// first time comparison if nextKey == lowKey, check for inclusiveness too
		getPosition(page, nextKey, lowKeyInclusive, type, position);
	}
	else{
		position = nextKeyOffset;

		// check for possible case of deletion while scanning [check for exact match]
		void* tempkey = malloc(PAGE_SIZE);
		int tempkeylen = 0;
		RID temprid;

		extractKey(page, position, type, tempkey, tempkeylen);
		memcpy(&temprid, (char*) page + position + tempkeylen, sizeof(RID));

		// look for exact matches
		if((compareKey(tempkey, nextKey, type) == 0) && (temprid.pageNum == nextKeyRid.pageNum) && (temprid.slotNum == nextKeyRid.slotNum)){
			position = nextKeyOffset;
		}
		else{
			// Dealing with duplicates, getting offset on the basis of <key,rid> pair
			getOffsetWithRID(page, nextKey, tempkeylen, nextKeyRid, type, position);
		}

		free(tempkey);
	}

	// Got the position, now check where position is less than PAGE_SIZE or not
	if(position <= PAGE_SIZE){

		short fs = 0;
		getFreeSpace(page, fs);

		short rightPgNumber = 0;
		getPointerToRight(page, rightPgNumber);
		if(position >= (PAGE_SIZE - fs) && rightPgNumber == -1){
			return -1;
		}


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
			free(page);
			free(rightPage);
			nextKeyBufferFlag = false;
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
				nextKeyBufferFlag = false;
				nextKeyPageNum = -1;
			}
			else{
				// right page exists
				// read the first key of this page and set it as nextKey
				ixfileHandle.fileHandle.readPage(rightPageNum, rightPage);

				// if right page is deleted page, go to further right sibling
				short rightPgEntries = 0;
				getEntryCount(rightPage, rightPgEntries);

				if(rightPgEntries == 0){

					getRightNonEmptyPageWithPageBuffer(ixfileHandle, rightPageNum, rightPage);

				}

				extractKey(rightPage, METADATA, type, nextKey, nextKeyLength);
				memcpy(&nextKeyRid, (char*) rightPage + METADATA + nextKeyLength, sizeof(RID));
				nextKeyPageNum = rightPageNum;
				nextKeyOffset = METADATA;

				//storing buffer for the nextKey
				memcpy(nextKeyBuffer, rightPage, PAGE_SIZE);
				nextKeyBufferFlag = true;
			}
		}
		else{

			// extract nextKey just after the hitKey

			// what if this was the last key
			short freespacethispage = 0;
			getFreeSpace(page, freespacethispage);

			if(nextposition >= (PAGE_SIZE - freespacethispage)){

				getPointerToRight(page, rightPageNum);

				// Small addition
				if(rightPageNum == -1){
					nextKeyBufferFlag = false;
					nextKeyPageNum = -1;
					free(page);
					free(rightPage);
					return 0;
				}

				ixfileHandle.fileHandle.readPage(rightPageNum, rightPage);

				// if right page is deleted page, go to further right sibling
				short rightPgEntries = 0;
				getEntryCount(rightPage, rightPgEntries);

				if(rightPgEntries == 0){

					getRightNonEmptyPageWithPageBuffer(ixfileHandle, rightPageNum, rightPage);

				}

				extractKey(rightPage, METADATA, type, nextKey, nextKeyLength);
				memcpy(&nextKeyRid, (char*) rightPage + METADATA + nextKeyLength, sizeof(RID));
				nextKeyPageNum = rightPageNum;
				nextKeyOffset = METADATA;

				//storing buffer for the nextKey
				memcpy(nextKeyBuffer, rightPage, PAGE_SIZE);
				nextKeyBufferFlag = true;
			}
			else{
				// bindaas extract karo
				extractKey(page, nextposition, type, nextKey, nextKeyLength);
				memcpy(&nextKeyRid, (char*) page + nextposition + nextKeyLength, sizeof(RID));
				getOwnPageNumber(page, nextKeyPageNum);
				nextKeyOffset = nextposition;

				//storing buffer for the nextKey
				memcpy(nextKeyBuffer, page, PAGE_SIZE);
				nextKeyBufferFlag = true;
			}
		}

	}
	else{
		// no hit in this page
		// No corner case, if the matching record is not in this page
		// That means, end of file reached.
		free(page);
		free(rightPage);
		return -1;
	}

	free(page);
	free(rightPage);
	// Hit found
	return 0;

}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	if(findHit(rid, key, attribute.type) == -1){
		iterationCount = 0;
		return -1;
	}

	return 0;
}

RC IX_ScanIterator::close()
{

	//TODO: complete this function

	if(lowKey){
		free(lowKey);
		lowKey = NULL;
	}
	if(nextKey){
		free(nextKey);
		nextKey = NULL;
	}
	if(highKey){
		free(highKey);
		highKey = NULL;
	}
	if(nextKeyBuffer){
		free(nextKeyBuffer);
		nextKeyBuffer = NULL;
	}

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

