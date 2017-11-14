
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

RC setRightPagePointer(void* data, short value){
	// 16-18 bytes
	int offset = 16;
	memcpy((char*)data + offset, &value, 2);
	return 0;
}

RC getRightPagePointer(void* data, short &value){
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
RC initializePage(IXFileHandle &ixfileHandle){
	/*
	 bytes: 0-2 : deletion_flag ( deleted = 0; else non-deleted page)
	 bytes: 2-4 : leaf/non-leaf flag (leaf_page = 0; else non_leaf page
	 bytes: 4-6 : level of the page in tree
	 bytes: 6-8 : number of entries in this page
	 bytes: 8-10: pointer to left sibling (unsigned page num)
	 bytes: 10-12: pointer to right sibling (unsigned page num)
	 bytes: 12-14: pointer to overflow pages (unsigned page num)
	 bytes: 14-16: free space in the given page
	 bytes: 16-18: store pageNum as the right ptr for non-leaf nodes
	 bytes: 18-20: Own page's pagenumber
	 bytes: 20-30: buffer (unused)
	 */
	short defaultValue = -1;

	void* data = malloc(PAGE_SIZE);

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
	//cout << "freespace : " << retVal << endl;

	//set right page ptr for non-leaf pages
	setRightPagePointer(data, defaultValue);

	//unit test case to test right page ptr
	getRightPagePointer(data, retVal);
	//cout << "right page ptr : " << retVal << endl;

	ixfileHandle.fileHandle.appendPage(data);
	short pageNumber = (short)(ixfileHandle.fileHandle.getNumberOfPages() - 1);
	setOwnPageNumber(data, pageNumber);
	getOwnPageNumber(data, pageNumber);
	//cout << "Own Page number : " << pageNumber << endl;
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
// Has left page pointer and a key
RC prepareNonLeafEntry(const short leftChildPageNum, const void* indexKey, void* indexEntry, AttrType type, const RID &rid, short &entrySize){

	switch(type){
		case TypeInt:{
			indexEntry = malloc(sizeof(short) + sizeof(int) + sizeof(RID));

			int offset = 0;

			memcpy((char*)indexEntry+offset, &leftChildPageNum, sizeof(short));
			offset += sizeof(short);

			memcpy((char*)indexEntry+offset, indexKey, sizeof(int));
			offset += sizeof(int);

			memcpy((char*)indexEntry+offset, &rid, sizeof(RID));
			offset += sizeof(RID);

			entrySize = offset;
			break;
		}
		case TypeReal:{
			indexEntry = malloc(sizeof(short) + sizeof(int) + sizeof(RID));

			int offset = 0;

			memcpy((char*)indexEntry+offset, &leftChildPageNum, sizeof(short));
			offset += sizeof(short);

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

			indexEntry = malloc(sizeof(short) + sizeof(int) + varCharSize + sizeof(RID));

			int offset = 0;
			memcpy((char*)indexEntry+offset, &leftChildPageNum, sizeof(short));
			offset += sizeof(short);

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

//Should return the place where it should insert
RC getOffset(void* page, const void* index_key, AttrType type, int &offsetToBeInserted){

	offsetToBeInserted = METADATA;

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	short leaf = 0;
	getLeafFlag(page, leaf);


	bool isLeaf = (leaf==LEAF_FLAG) ? true : false;
	//cout<< "IS leaf : " << isLeaf<<endl;

	switch(type){
		case TypeInt:{

			//read the index key from index entry
			int searchKey = 0;
			memcpy(&searchKey, index_key, sizeof(int));

			//offset initialized to METADATA
			//cout << "Initial offset : " << offsetToBeInserted << endl;


			int key = 0;

			if(isLeaf){
				//leaf - 12 bytes
				// key + rid
				int leafPageEntrySize = sizeof(int) + (sizeof(RID));

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted, sizeof(int));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += leafPageEntrySize;
				}
			}
			else{
				//non-leaf - 6 bytes
				int nonLeafPageEntrySize = sizeof(short) + sizeof(int);					// left child + key

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted+sizeof(short), sizeof(int));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}

			break;
		}

		case TypeReal:{

			//read the index key from index entry
			float searchKey = 0.0f;
			memcpy(&searchKey, index_key, sizeof(float));

			//offset initialized to METADATA
			//cout << offsetToBeInserted << endl;


			float key = 0.0f;

			if(isLeaf){
				//leaf - 12 bytes
				// key + rid
				int leafPageEntrySize = sizeof(float) + (sizeof(RID));

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted, sizeof(float));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += leafPageEntrySize;
				}
			}
			else{
				//non-leaf - 6 bytes
				// left child + key
				int nonLeafPageEntrySize = sizeof(short) + sizeof(float);

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted+sizeof(short), sizeof(float));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}

			break;
		}

		case TypeVarChar:{

			//read the index key from index entry
			int searchKeyLength = 0;
			memcpy(&searchKeyLength, index_key, sizeof(int));

			char* searchKey = (char*) malloc(searchKeyLength+1);
			memcpy(searchKey, (char*)index_key+sizeof(int), searchKeyLength);
			searchKey[searchKeyLength] = '\0';

			//offset initialized to METADATA
			//cout << offsetToBeInserted << endl;


			if(isLeaf){
				//variable length Leaf Page

				for(int i=0; i<entryCount; i++){

					// key length extraction from the entry
					int keyLength = 0;
					memcpy(&keyLength, (char*)page+offsetToBeInserted, sizeof(int));

					// Varcharkey + rid
					int leafPageEntrySize = sizeof(int) + keyLength + sizeof(RID);

					// key extraction from the entry
					char* key = (char*) malloc(keyLength+1);
					memcpy(key, (char*)page+offsetToBeInserted+sizeof(int), keyLength);
					key[keyLength] = '\0';

					//compare
					if(strcmp(key,searchKey) >= 0){
						break;
					}

					//increase offset
					offsetToBeInserted += leafPageEntrySize;
				}
			}
			else{
				//variable length Non-Leaf Page

				for(int i=0; i<entryCount; i++){

					int keyLength = 0;
					memcpy(&keyLength, (char*)page+offsetToBeInserted+sizeof(short), sizeof(int));

					char* key = (char*) malloc(keyLength+1);
					memcpy(key, (char*)page + offsetToBeInserted + sizeof(short) + sizeof(int), keyLength);
					key[keyLength] = '\0';

					// left child + Varcharkey
					int nonLeafPageEntrySize = sizeof(short) + sizeof(int) + keyLength;

					if(strcmp(key,searchKey) >= 0){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}
			break;
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

	int testKey = 0;
	memcpy(&testKey, entry, sizeof(int));
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

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	short rootPageNum = (short)ixfileHandle.fileHandle.getCounter(5);

	// First entry ever
	if(rootPageNum == NO_ROOT){
		// Create your page meta data - till now 30 bytes
		initializePage(ixfileHandle);
		void* page_data = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(0, page_data);

		//prepare leaf entry
		void* entry = malloc(PAGE_SIZE);
		short entrySize = 0;

		prepareLeafEntry(key, entry, attribute.type, rid, entrySize);

		//add entry in a page iff there is space
		short freeSpace = 0;
		getFreeSpace(page_data, freeSpace);
		if(freeSpace >= entrySize){

			//offsetToBeInserted shld be logically 0 for getOffset function
			int offsetToBeInserted = METADATA;

			//insertion sort
			getOffset(page_data, key, attribute.type, offsetToBeInserted);
			//cout << "Returned offset to be inserted : " << offsetToBeInserted << endl;

			//write into buffer - shift right to create space and insert entry in sorted location

			writeIntoPageBuffer(page_data, entry, entrySize, offsetToBeInserted);

			// Extract own page Number
			short pageNumber = 0;
			getOwnPageNumber(page_data, pageNumber);


			//write into file
			writeIntoPage(ixfileHandle, page_data, pageNumber);


			//updating the root page number
			ixfileHandle.fileHandle.setCounter(5, pageNumber);

		}
		else{

			//split


			//write into a leaf node



			//update the metadata i.e decrement the freespace


			//write into a non-leaf node = ROOT CREATION
			//replace the root pointer in the hidden page - call set with mode 5


			//update the metadata i.e decrement the freespace

		}


	}
	else{

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
    return 0;
}

short traverse(const void* searchKey, bool searchKeyInclusive, const int rootPageNum){

	short pageNum = 0;

	//if NO ROOT
	if(rootPageNum == 0){
		return rootPageNum;
	}

	//else ROOT

	return pageNum;
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

	short pageNum = traverse(key, true, rootPageNum);

	ixfileHandle.fileHandle.readPage(pageNum, page);

	//getOffset()
	int offset = 0;
	getOffset(page, key, attribute.type, offset);


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

RC setKeys(const void *key, AttrType type, void* buffer){



	switch(type){

		case TypeInt:{
			memcpy(buffer, key, sizeof(int));

			break;
		}

		case TypeReal:{
			memcpy(buffer, key, sizeof(int));

			break;
		}

		case TypeVarChar:{
			int varCharLength = 0;
			memcpy(&varCharLength, key, sizeof(int));
			memcpy(buffer, key, sizeof(int) + varCharLength);

			break;
		}
	}

	return 0;
}

RC searchLeftmostLeafNode(IXFileHandle &ixfileHandle, void *lowKey, AttrType type){

	short rootPageNum = (short)ixfileHandle.fileHandle.getCounter(5);

	if(rootPageNum == 0){
		if(ixfileHandle.fileHandle.getNumberOfPages() == 0){
			return -1;
		}
		else{
			//get entryCount
			void* page = malloc(PAGE_SIZE);
			ixfileHandle.fileHandle.readPage(0, page);

			//test
			int temp = 0;
			memcpy(&temp, (char*)page+METADATA, sizeof(int));
			//cout<<"FKEY---"<<temp<<endl;
			//test end

			int offset = METADATA;

			switch(type){
				//handling int and real
				case TypeInt:{
					//lowKey = malloc(sizeof(int));
					memcpy(lowKey, (char*)page + offset, sizeof(int));
					break;
				}

				case TypeReal:{
					//lowKey = malloc(sizeof(int));
					memcpy(lowKey, (char*)page+offset, sizeof(float));
					break;
				}
				//handling varchar
				case TypeVarChar:{
					int Varlength = 0;
					memcpy(&Varlength, (char*)page+offset, sizeof(int));
					//lowKey = malloc(sizeof(int) + Varlength);

					memcpy(lowKey, (char*)page+offset, sizeof(int) + Varlength);

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

	ix_ScanIterator.ixfileHandle = &ixfileHandle;



//	ix_ScanIterator.attribute.length = attribute.length;
//	ix_ScanIterator.attribute.name = attribute.name;
//	ix_ScanIterator.attribute.type = (AttrType)attribute.type;
	ix_ScanIterator.attribute = &attribute;


	//cout<<"Inside scan"<<endl;

	ix_ScanIterator.lowKey = malloc(PAGE_SIZE);
	if(lowKey){
		setKeys(lowKey, attribute.type, ix_ScanIterator.lowKey);
		ix_ScanIterator.lowKeyInclusive = lowKeyInclusive;
	}
	else{
		ix_ScanIterator.lowKeyInclusive = true;


		//TODO: if page 0 is empty

		ix_ScanIterator.nextKeyPageNum = 0;



		if(searchLeftmostLeafNode(ixfileHandle, ix_ScanIterator.lowKey, attribute.type)==-1){
			return -1;
		}
	}

	ix_ScanIterator.highKey = malloc(PAGE_SIZE);
	if(highKey){
		setKeys(highKey, attribute.type, ix_ScanIterator.highKey);
		ix_ScanIterator.highKeyInclusive = highKeyInclusive;

	}
	else{
		ix_ScanIterator.highKeyInclusive = true;
		if(searchRightmostLeafNode(ixfileHandle, ix_ScanIterator.highKey, attribute.type)==-1){
			return -1;
		}
	}


	int key = 0;
	memcpy(&key, ix_ScanIterator.lowKey, sizeof(int));
	//cout<<"Low: "<<key<<endl;

	memcpy(&key, ix_ScanIterator.highKey, sizeof(int));
	//cout<<"High: " <<key<<endl;

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
/*
	switch(type){
		case TypeInt:{

			//read the index key from index entry
			int searchKey = 0;
			memcpy(&searchKey, index_key, sizeof(int));

			//offset initialized to METADATA
			//cout << "Initial offset : " << offsetToBeInserted << endl;


			int key = 0;

			if(isLeaf){

			}
			else{
				//non-leaf - 6 bytes
				int nonLeafPageEntrySize = sizeof(short) + sizeof(int);					// left child + key

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted+sizeof(short), sizeof(int));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}

			break;
		}

		case TypeReal:{

			//read the index key from index entry
			float searchKey = 0.0f;
			memcpy(&searchKey, index_key, sizeof(float));

			//offset initialized to METADATA
			//cout << offsetToBeInserted << endl;


			float key = 0.0f;

			if(isLeaf){
				//leaf - 12 bytes
				// key + rid
				int leafPageEntrySize = sizeof(float) + (sizeof(RID));

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted, sizeof(float));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += leafPageEntrySize;
				}
			}
			else{
				//non-leaf - 6 bytes
				// left child + key
				int nonLeafPageEntrySize = sizeof(short) + sizeof(float);

				//search for the correct position
				for(int i=0; i<entryCount; i++){
					memcpy(&key, (char*)page+offsetToBeInserted+sizeof(short), sizeof(float));

					if(key >= searchKey){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}

			break;
		}

		case TypeVarChar:{

			//read the index key from index entry
			int searchKeyLength = 0;
			memcpy(&searchKeyLength, index_key, sizeof(int));

			char* searchKey = (char*) malloc(searchKeyLength+1);
			memcpy(searchKey, (char*)index_key+sizeof(int), searchKeyLength);
			searchKey[searchKeyLength] = '\0';

			//offset initialized to METADATA
			cout << offsetToBeInserted << endl;


			if(isLeaf){
				//variable length Leaf Page

				for(int i=0; i<entryCount; i++){

					// key length extraction from the entry
					int keyLength = 0;
					memcpy(&keyLength, (char*)page+offsetToBeInserted, sizeof(int));

					// Varcharkey + rid
					int leafPageEntrySize = sizeof(int) + keyLength + sizeof(RID);

					// key extraction from the entry
					char* key = (char*) malloc(keyLength+1);
					memcpy(key, (char*)page+offsetToBeInserted+sizeof(int), keyLength);
					key[keyLength] = '\0';

					//compare
					if(strcmp(key,searchKey) >= 0){
						break;
					}

					//increase offset
					offsetToBeInserted += leafPageEntrySize;
				}
			}
			else{
				//variable length Non-Leaf Page

				for(int i=0; i<entryCount; i++){

					int keyLength = 0;
					memcpy(&keyLength, (char*)page+offsetToBeInserted+sizeof(short), sizeof(int));

					char* key = (char*) malloc(keyLength+1);
					memcpy(key, (char*)page + offsetToBeInserted + sizeof(short) + sizeof(int), keyLength);
					key[keyLength] = '\0';

					// left child + Varcharkey
					int nonLeafPageEntrySize = sizeof(short) + sizeof(int) + keyLength;

					if(strcmp(key,searchKey) >= 0){
						break;
					}
					offsetToBeInserted += nonLeafPageEntrySize;
				}
			}
			break;
		}
	}

	return 0;
	*/
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


RC IX_ScanIterator::getPosition(void* page, const void* nextkey, bool lowKeyInclusive, AttrType type, int &position){

	//get number of index entries
	short entryCount = 0;
	getEntryCount(page, entryCount);

	position = METADATA;

	int found = -1;

	switch(type){
		case TypeInt:{

			//read the index key from index entry
			int searchKey = 0;
			memcpy(&searchKey, nextkey, sizeof(int));

			int key = 0;


			//leaf - 12 bytes
			// key + rid
			int leafPageEntrySize = sizeof(int) + (sizeof(RID));

			//search for the correct position
			for(int i=0; i<entryCount; i++){
				memcpy(&key, (char*)page+position, sizeof(int));

				if(lowKeyInclusive){
					if(key >= searchKey){
						found = 0;
						break;
					}
				}
				else{
					if(key > searchKey){
						found = 0;
						break;
					}
				}

				position += leafPageEntrySize;
			}


			break;
		}

		case TypeReal:{

			//read the index key from index entry
			float searchKey = 0.0f;
			memcpy(&searchKey, nextkey, sizeof(float));

			float key = 0.0f;

			//leaf - 12 bytes
			// key + rid
			int leafPageEntrySize = sizeof(float) + (sizeof(RID));

			//search for the correct position
			for(int i=0; i<entryCount; i++){
				memcpy(&key, (char*)page+position, sizeof(float));

				if(lowKeyInclusive){
					if(key >= searchKey){
						found = 0;
						break;
					}
				}
				else{
					if(key > searchKey){
						found = 0;
						break;
					}
				}

				position += leafPageEntrySize;
			}

			break;
		}

		case TypeVarChar:{

			//read the index key from index entry
			int searchKeyLength = 0;
			memcpy(&searchKeyLength, nextkey, sizeof(int));

			char* searchKey = (char*) malloc(searchKeyLength+1);
			memcpy(searchKey, (char*)nextkey+sizeof(int), searchKeyLength);
			searchKey[searchKeyLength] = '\0';

//			//offset initialized to METADATA
//			cout << offsetToBeInserted << endl;


			//variable length Leaf Page

			for(int i=0; i<entryCount; i++){

				// key length extraction from the entry
				int keyLength = 0;
				memcpy(&keyLength, (char*)page+position, sizeof(int));

				// Varcharkey + rid
				int leafPageEntrySize = sizeof(int) + keyLength + sizeof(RID);

				// key extraction from the entry
				char* key = (char*) malloc(keyLength+1);
				memcpy(key, (char*)page+position+sizeof(int), keyLength);
				key[keyLength] = '\0';

				//compare
				if(lowKeyInclusive){
					if(strcmp(key,searchKey) >= 0){
						found = 0;
						break;
					}
				}
				else{
					if(strcmp(key,searchKey) > 0){
						found = 0;
						break;
					}
				}

				//increase offset
				position += leafPageEntrySize;
			}

			break;
		}
	}

	return found;
}

RC IX_ScanIterator::findHitForTypeInt(RID &rid, void* key){

	if(nextKeyPageNum == -1){
		return -1;
	}

	//fetch root page num
	int rootPageNum = ixfileHandle->fileHandle.getCounter(5);
	nextKeyPageNum = traverse(nextKey, true, rootPageNum);

	while(true){

		//reading next key page
		void* page = malloc(PAGE_SIZE);
		ixfileHandle->fileHandle.readPage(nextKeyPageNum, page);

		//get the position of lowKey i.e start of matching entries
		int position = 0;
		if(getPosition(page, nextKey, lowKeyInclusive, TypeInt, position) == -1){
			return -1;
		}

		//extract high key i.e till where to match
		int highKEY = 0;
		memcpy(&highKEY, highKey, sizeof(int));

		//scanning nextKey
		while(position <= PAGE_SIZE){

			//nextKey
			int searchKey = 0;
			memcpy(&searchKey, (char*)page+position, sizeof(int));

			//compare with high key
			if(highKeyInclusive){
				//inclusive
				if(searchKey <= highKEY){
					//return 'hit' RIDs and key
					memcpy(&rid, (char*)page+position+sizeof(int), sizeof(RID));
					memcpy(key, &searchKey, sizeof(int));

					//-set nextKey for next call
					//increment the position
					position += sizeof(int) + sizeof(RID);

					//chk for next page
					if(position >= PAGE_SIZE){

						//get the right page number
						short rightPageNum = 0;
						getPointerToRight(page, rightPageNum);

						//reached end of the B+ tree
						if(rightPageNum == -1){
							nextKeyPageNum = -1;
						}
						//right page exists
						//read the first key of this page
						else{
							void* rightPage = malloc(PAGE_SIZE);
							ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);

							memcpy(nextKey, (char*)rightPage+METADATA, sizeof(int));
							nextKeyPageNum = rightPageNum;
						}
					}
					else{
						//add breaking condition
						short entriesRead = (position-METADATA)/(sizeof(int)+sizeof(RID));
						short noOfEntries = 0;
						getEntryCount(page, noOfEntries);
						if(entriesRead < noOfEntries){
							memcpy(nextKey, (char*)page+position, sizeof(int));
						}
						else{
							nextKeyPageNum = -1;
						}
					}

					// return hit
					return 0;
				}
				else{
					// no hit till now
					return -1;
				}
			}
			else{
				//exclusive
				if(searchKey < highKEY){
					//return 'hit' RIDs and key
					memcpy(&rid, (char*)page+position+sizeof(int), sizeof(RID));
					memcpy(key, &searchKey, sizeof(int));

					//-set nextKey for next call

					//increment the position
					position += sizeof(int) + sizeof(RID);

					//chk for next page
					if(position >= PAGE_SIZE){

						//get the right page number
						short rightPageNum = 0;
						getPointerToRight(page, rightPageNum);

						//reached end of the B+ tree
						if(rightPageNum == -1){
							nextKeyPageNum = -1;
						}
						//right page exists
						//read the first key of this page
						else{
							void* rightPage = malloc(PAGE_SIZE);
							ixfileHandle->fileHandle.readPage(rightPageNum, rightPage);

							memcpy(nextKey, (char*)rightPage+METADATA, sizeof(int));
							nextKeyPageNum = rightPageNum;
						}
					}
					else{
						//add breaking condition
						short entriesRead = (position-METADATA)/(sizeof(int)+sizeof(RID));
						short noOfEntries = 0;
						getEntryCount(page, noOfEntries);
						if(entriesRead < noOfEntries){
							memcpy(nextKey, (char*)page+position, sizeof(int));
						}
						else{
							nextKeyPageNum = -1;
						}
					}

					return 0;
				}
				else{
					return -1;
				}
			}

			position += sizeof(int) + sizeof(RID);
		}

		//getPointerToRight(page, nextKeyPageNum);
	}

	return 0;
}

RC IX_ScanIterator::findHitForTypeReal(RID &rid, void* key){

	void* page = malloc(PAGE_SIZE);
	ixfileHandle->fileHandle.readPage(nextKeyPageNum, page);

	int position = 0;

	if(getPosition(page, lowKey, lowKeyInclusive, TypeInt, position) == -1){
		return -1;
	}




	return 0;
}

RC IX_ScanIterator::findHitForTypeVarChar(RID &rid, void* key){
	void* page = malloc(PAGE_SIZE);
	ixfileHandle->fileHandle.readPage(nextKeyPageNum, page);

	int position = 0;

	if(getPosition(page, lowKey, lowKeyInclusive, TypeInt, position) == -1){
		return -1;
	}



	return 0;
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	int lowkeyVal = 0;
	memcpy(&lowkeyVal, lowKey, sizeof(int));
	//cout<<"In getNextEntry: "<<lowkeyVal<<endl;

	switch(attribute->type){
		case TypeInt:{
			if(nextKey == NULL){
				nextKey = malloc(sizeof(int));
				memcpy(nextKey, lowKey, sizeof(int));
			}
			if(findHitForTypeInt(rid, key) == 0){
				return 0;
			}
			else{
				return -1;
			}
			break;
		}

		case TypeReal:{
			if(nextKey == NULL){
				nextKey = malloc(sizeof(float));
				memcpy(nextKey, lowKey, sizeof(float));
			}
			if(findHitForTypeReal(rid, key) == 0){
				return 0;
			}
			else{
				return -1;
			}
			break;
		}

		case TypeVarChar:{
			if(nextKey == NULL){
				int size = 0;
				memcpy(&size, lowKey, sizeof(int));
				nextKey = malloc(sizeof(int)+size);
				memcpy(nextKey, lowKey, sizeof(int)+size);

			}
			if(findHitForTypeVarChar(rid, key) == 0){
				return 0;
			}
			else{
				return -1;
			}
			break;
		}
	}
    return -1;
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

