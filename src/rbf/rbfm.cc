#include "rbfm.h"

#include <stdlib.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
	PagedFileManager* pfm = PagedFileManager::instance();
	return pfm->closeFile(fileHandle);
}

RC insertRecordInGivenBuffer(void *pageBuffer, char* record, int recordSize, int* slot){
	//slots in page
	short int slots = -1;
	memcpy(&slots, pageBuffer+PAGE_SIZE-4, sizeof(short int));

	//offset to the last slot in slot dir
	int offsetLastSlotInDir = PAGE_SIZE-4-(4*(slots));
	int offsetNewRecord = -1;

	if(offsetLastSlotInDir==PAGE_SIZE-4){
		offsetNewRecord = 0;
	}
	else{
		//reading the last slot info
		short int lastSlot[2];
		//GET THE VALUES OF THE LAST SLOT IN THE PAGE TO GET THE OFFSET OF NEW RECORD TO BE WRITTEN
		memcpy(lastSlot, pageBuffer+offsetLastSlotInDir, 2*sizeof(short int));

		offsetNewRecord = lastSlot[0]+lastSlot[1];
	}

	//Copying the record into the page buffer
	//memcpy(pageBuffer+offsetNewRecord, record, recordSize);


	//update slot dir
	short int newSlot[2];
	newSlot[0] = offsetNewRecord;
	newSlot[1] = recordSize;
	memcpy(pageBuffer+(offsetLastSlotInDir-4), newSlot, 2*sizeof(short int));

	//update freespace and no of slots
	short int lastBlock[2];
	memcpy(lastBlock, pageBuffer+PAGE_SIZE-4, 2*sizeof(short int));

	//updating no of slots
	lastBlock[0] = lastBlock[0]+1;

	//updating free space
	lastBlock[1] = lastBlock[1]-(recordSize+4);
	memcpy(pageBuffer+PAGE_SIZE-4, lastBlock, 2*sizeof(short int));

	*slot = slots;

	return 0;
}

RC createRecord(const vector<Attribute> &recordDescriptor, const void *data, char *record, int *recordSize, int* ptrs, int* actualdata){
	int fields = recordDescriptor.size();

	int noOfNULLBytes = ceil((double) fields / 8.0);

//	//copying the no. of records into record
//	memcpy(record, &N, sizeof(int));

	//INCREASE THE SIZE OF THE RECORD
//	*recordSize += sizeof(int);

	//copy null bytes into record
	memcpy((*recordSize)+record, data, noOfNULLBytes);

	//INCREASE THE SIZE OF THE RECORD
	*recordSize += noOfNULLBytes;

	//GET BIT VECTOR FROM NULL BYTES
	int bitArray[noOfNULLBytes*8];

	char* byte = (char*)malloc(noOfNULLBytes);
	memcpy(byte, record, noOfNULLBytes);

	for(int i=0; i<noOfNULLBytes*8; i++){
		bitArray[i] = byte[i/8]&(1<<(7-(i%8)));
	}

	int ptrsCnt = 0;
	// calculate non null attr
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			ptrsCnt++;
		}
	}


	//pointers to the end of field
	char* endOfField = (char*)malloc(ptrsCnt*sizeof(short int));

	//copy the ptrs in record
	memcpy(record+(*recordSize), endOfField, ptrsCnt*sizeof(short int));

	//update record size
	*recordSize += ptrsCnt*sizeof(short int);

	//offset -> blocks of ptrs
	int offset_ptrs = noOfNULLBytes;

	//pts to offset dabba
	*ptrs = offset_ptrs;

	int offset_actual_data = *recordSize;

	*actualdata = offset_actual_data;

	//data offset currently being read
	int offset_data_curr = noOfNULLBytes;

	//reading data and recordDescriptor to populate actual data in records
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){
			case TypeVarChar:{
				//read string length from data
				int len = -1;
				memcpy(&len, data+offset_data_curr, sizeof(int));

				//increase the data offset
				offset_data_curr += sizeof(int);

				//read the string from the data
				char* str = (char*)malloc(len);
				memcpy(str, data+offset_data_curr, len);

				//increase the data offset
				offset_data_curr += len;

				//write actual field value into record
				memcpy(record+offset_actual_data, str, len);
				offset_actual_data += len;

				//update offset ptr
				memcpy(record+offset_ptrs, &offset_actual_data, sizeof(short int));
				offset_ptrs += sizeof(short int);

				free(str);

				break;
			}
			case TypeInt:{
				int val = -1;

				//copy from data
				memcpy(&val, data+offset_data_curr, sizeof(int));

				//increase data pos
				offset_data_curr += sizeof(int);

				//copy to record
				memcpy(record+offset_actual_data, &val, sizeof(int));

				//increase the actual data offset
				offset_actual_data += sizeof(int);

				//update the field ptr
				memcpy(record+offset_ptrs, &offset_actual_data, sizeof(short int));
				offset_ptrs += sizeof(short int);

				break;
			}
			case TypeReal:{
				memcpy(record+offset_actual_data, data+offset_data_curr, sizeof(float));
				offset_data_curr += sizeof(float);

				//increase the actual data offset
				offset_actual_data += sizeof(float);

				//update the field ptr
				memcpy(record+offset_ptrs, &offset_actual_data, sizeof(short int));
				offset_ptrs += sizeof(short int);

			}
			}
		}

	}
	*recordSize = offset_actual_data;

	free(byte);
	free(endOfField);

	return -1;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	int recordSize = 0;
	int ptrs = 0;
	int actualdata = 0;
	char* record = (char*)malloc(4096);

	//create record
	createRecord(recordDescriptor, data, record, &recordSize, &ptrs, &actualdata);

	//page to be written
	int PAGE = -1;
	int SLOT = -1;
	int flag = 0;
	//--check if the file contains any pages
	//--if not create a page and initialize the structure
	int noOfPgs = fileHandle.getNumberOfPages();

	if(noOfPgs == 0){
		void* dummy = malloc(PAGE_SIZE);

		//initializing the free space and slot numbers
		short int st[2];
		st[0] = 0;
		st[1] = PAGE_SIZE-4;

		//write it to dummy buffer
		memcpy(dummy+PAGE_SIZE-4, st, 2*sizeof(short int));


		fileHandle.appendPage(dummy);
		PAGE = 0;
		flag = 1;
		free(dummy);
	}
	else{
		int lastPage = noOfPgs-1;

		//TEMPORARY BUFFER TO STORE THE CONTENTS OF THE LAST PAGE
		void* pageBufferLastPage = malloc(PAGE_SIZE);

		//READ THE FILE CONTENTS INTO TEMPORARY BUFFER
		fileHandle.readPage(lastPage, pageBufferLastPage);

		//CHECK THE AVAILABE SPACE IN THE PAGE
		short int freespaceLastPage = -1;
		memcpy(&freespaceLastPage, pageBufferLastPage+PAGE_SIZE-2, sizeof(short int));

		free(pageBufferLastPage);

		if(recordSize+2*sizeof(short int)<=freespaceLastPage){
			flag = 1;
			PAGE = lastPage;
		}
		else{
			void* pageBuffer = malloc(lastPage*PAGE_SIZE);
			fseek(fileHandle.handle, PAGE_SIZE, SEEK_SET);
			fread(pageBuffer, lastPage*PAGE_SIZE, 1, fileHandle.handle);

			for(int i=0; i<lastPage; i++){
				//compute free space

				//CHECK THE AVAILABE SPACE IN THE PAGE
				short int freespace = 0;
				memcpy(&freespace, pageBuffer+((i+1)*PAGE_SIZE)-sizeof(short int), sizeof(short int));

				if(recordSize+8<=freespace){
					flag = 1;
					PAGE = i;
					break;
				}
			}
			free(pageBuffer);
		}
	}

	if(flag==0){
		//append at the last
		void* dummy = malloc(4096);

		//initializing the free space and slot numbers
		short int st[2];
		st[0] = 0;
		st[1] = PAGE_SIZE - 4;

		//write it to dummy buffer
		memcpy(dummy+PAGE_SIZE-4, st, 2*sizeof(short int));

		fileHandle.appendPage(dummy);
		PAGE = noOfPgs;
		free(dummy);
	}

	//Read the page from file
	void* pageBuffer = malloc(PAGE_SIZE);
	fileHandle.readPage(PAGE, pageBuffer);

	//call insertRecordBufferwala
	insertRecordInGivenBuffer(pageBuffer, record, recordSize, &SLOT);

	//write the new page to the file
	fileHandle.writePage(PAGE, pageBuffer);

	//update the RECORD ID
	rid.pageNum = PAGE;
	rid.slotNum = SLOT;

	free(pageBuffer);
	free(record);

	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	//GET DATA FROM RID
	int pgNum = rid.pageNum;
	int slotNum = rid.slotNum;

	//READ THE PAGE FROM THE GIVEN FILE HANDLE
	char* pageBuffer = (char*)malloc(PAGE_SIZE);
	fileHandle.readPage(pgNum, pageBuffer);

	//READ THE BUFFER
	short int slots = -1;
	memcpy(&slots, pageBuffer+PAGE_SIZE-4, sizeof(short int));


	//CALCULATE OFFSET IN SLOT DIRECTORY
	//slotNum are 0-indexed, so +1
	int offset = PAGE_SIZE-4 - (4*(slotNum+1));

	//READ THE SLOT INFO
	short int slotInfo[2];
	memcpy(slotInfo, pageBuffer+offset, 2*sizeof(short int));

	//CHK
	short int offsetFromOrigin = slotInfo[0];
	short int slotSize = slotInfo[1];


	//READ THE RECORD FROM THE PAGE
	char* recordBuffer = (char*)malloc(slotSize);
	memcpy(recordBuffer, pageBuffer+offsetFromOrigin, slotSize);

	//CHK
//	float val = 0.0f;
//	memcpy(&val, recordBuffer, sizeof(float));
//	cout<<"lol--->"<<val<<endl;

	//WRITE THE RECORD INTO data buffer in the given format
	int fields = recordDescriptor.size();
	int offsetRead = 0;
	int offsetWrite = 0;

	//memcpy(&fields, recordBuffer+offsetRead, sizeof(int));

	int nullBytes = ceil((double) fields / 8.0);

	//COPY THE NULL BYTES INTO THE data buffer
	memcpy(data+offsetWrite, recordBuffer+offsetRead, nullBytes);

	offsetWrite+=nullBytes;
	offsetRead+=nullBytes;

	//GET BIT VECTOR FROM NULL BYTES
	int bitArray[nullBytes*8];

	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, recordBuffer, nullBytes);


	for(int i=0; i<nullBytes*8; i++){
		bitArray[i] = byte[i/8]&(1<<(7-(i%8)));
	}

	//calculate size of non null attr
	int ptrsCnt = 0;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			ptrsCnt++;
		}
	}


	//COPY ACTUAL FIELD DATA INTO THE data buffer
	int offsetPtrs = offsetRead;
	int offsetActualData = offsetPtrs+(ptrsCnt*sizeof(short int));


	offsetRead = offsetActualData;

	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){
			case TypeVarChar:{
				short int endVal = 0;

				//GET THE END POSITION OF THE FIELD
				memcpy(&endVal ,recordBuffer+offsetPtrs, sizeof(short int));
				offsetPtrs += sizeof(short int);

				//CALCULATE SIZE OF THE STRING
				int size = ((int)endVal) - offsetActualData;

				//WRITE THE SIZE INTO data buffer
				memcpy(data+offsetWrite, &size, sizeof(int));
				offsetWrite += sizeof(int);


				//WRITE THE STRING INTO data buffer
				memcpy(data+offsetWrite, recordBuffer+offsetActualData, size);
				offsetActualData += size;
				offsetWrite += size;

				break;
			}
			case TypeInt:{

				offsetPtrs += sizeof(short int);

				//WRITE THE INT INTO data buffer
				memcpy(data+offsetWrite, recordBuffer+offsetActualData, sizeof(int));
				offsetActualData += sizeof(int);
				offsetWrite += sizeof(int);

				break;
			}
			case TypeReal:{

				offsetPtrs += sizeof(short int);

				//WRITE THE REAL INTO data buffer
				memcpy(data+offsetWrite, recordBuffer+offsetActualData, sizeof(float));
				offsetActualData += sizeof(float);
				offsetWrite += sizeof(float);

				break;
			}
			}
		}
	}

	free(byte);
	free(recordBuffer);
	free(pageBuffer);
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {

	int fields = recordDescriptor.size();

	int nullBytes = ceil((double) fields / 8.0);

	//GET BIT VECTOR FROM NULL BYTES
	int bitArray[nullBytes*8];

	int offsetDataRead = 0;

	//copy the null bytes
	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, data, nullBytes);
	//cout<<"copying done"<<endl;

	offsetDataRead+=nullBytes;
	//cout<<"Offset data"<<offsetDataRead<<endl;

	for(int i=0; i<nullBytes*8; i++){
		bitArray[i] = byte[i/8]&(1<<(7-(i%8)));
	}

	free(byte);

	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

			case TypeVarChar:{
				int size = 0;
				memcpy(&size, data+offsetDataRead, sizeof(int));

				offsetDataRead += sizeof(int);

				char* str = (char*)malloc(sizeof(char)*size+1);
				memcpy(str, data+offsetDataRead, size);
				str[size] = '\0';
				offsetDataRead += size;

				cout<<recordDescriptor[i].name<<": "<<str<<"\t";
				free(str);

				break;
			}

			case TypeInt:{
				int val = 0;
				memcpy(&val, data+offsetDataRead, sizeof(int));
				offsetDataRead += sizeof(int);
				cout<<recordDescriptor[i].name<<": "<<val<<"\t";
				break;
			}

			case TypeReal:{
				float val1 = 0.0f;
				memcpy(&val1, data+offsetDataRead, sizeof(float));
				offsetDataRead += sizeof(float);
				cout<<recordDescriptor[i].name<<": "<<val1<<"\t";
				break;
			}
			}
		}
		else{
			cout<<recordDescriptor[i].name<<": "<<"NULL"<<"\t";
		}
	}
	cout<<endl;
	return 0;
}
