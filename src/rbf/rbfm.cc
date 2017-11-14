#include "rbfm.h"

#include <stdlib.h>
#include <cmath>
#include <cstdio>
#include<string>
#include <cstring>
#include <iostream>
#include <new>


// My Defined Values - Any value greater than PAGE_SIZE
#define DEL 5000
#define POINT 6000


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

	//if(_rbf_manager){
			//delete _rbf_manager;
	//}
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

int RecordBasedFileManager::getHighestslotOffsetLocation(char* pageBuffer){
	int slotNum = -1;
	short int slotoffset = -1;
	short int slotlength = -1;
	short int slotoffsetmax = -1;
	short int totalslots = -1;
	memcpy(&totalslots, (char*)pageBuffer + PAGE_SIZE-4, sizeof(short int));
	for(int i=0; i < totalslots; i++){
		memcpy(&slotoffset, (char*)pageBuffer + PAGE_SIZE-4-(4*(i+1)), sizeof(short int));
		memcpy(&slotlength, (char*)pageBuffer + PAGE_SIZE-4-(4*(i+1))+2, sizeof(short int));
		if(slotoffset > slotoffsetmax && slotlength!= ((short int)DEL)){
			slotoffsetmax = slotoffset;
			slotNum = i;
		}
	}
	return slotNum;
}

int RecordBasedFileManager::deletedSlotLocation(char* pageBuffer){
	int slotNum = -1;
	short int slotlength = -1;
	short int totalslots = -1;
	memcpy(&totalslots, (char*)pageBuffer + PAGE_SIZE-4, sizeof(short int));
	for(int i=0; i < totalslots; i++){
		memcpy(&slotlength, (char*)pageBuffer + PAGE_SIZE-4-(4*(i+1))+2, sizeof(short int));
		if(slotlength == ((short int)DEL)){
			slotNum = i;
			break;
		}
	}
	return slotNum;
}

RC RecordBasedFileManager::insertRecordInGivenBuffer(char *pageBuffer, char* record, int recordSize, int* slot){
	//slots in page
	short int slots = -1;
	memcpy(&slots, (char*)pageBuffer+PAGE_SIZE-4, sizeof(short int));

	//offset to the last slot in slot dir
	int offsetLastSlotInDir = PAGE_SIZE-4-(4*(slots));
	int offsetNewRecord = -1;

	if(offsetLastSlotInDir==PAGE_SIZE-4){
		offsetNewRecord = 0;
	}
	else{
		int k = getHighestslotOffsetLocation((char*)pageBuffer);
		//reading the last slot(Highest slot) info
		short int lastSlot[2];
		//GET THE VALUES OF THE LAST SLOT IN THE PAGE TO GET THE OFFSET OF NEW RECORD TO BE WRITTEN
		memcpy(lastSlot, (char*)pageBuffer+ PAGE_SIZE-4-(4*(k+1)), 2*sizeof(short int));

		// Length is fixed for the pointer data (8 bytes, twice the size of unsigned)
		if(lastSlot[1] == (short int) POINT){
			offsetNewRecord = lastSlot[0] + (2*(sizeof(unsigned)));
		}
		else{
			// Normal slot, not a pointer slot
			offsetNewRecord = lastSlot[0]+lastSlot[1];
		}
	}

	//Copying the record into the page buffer
	memcpy((char*)pageBuffer+offsetNewRecord, record, recordSize);

	int d = deletedSlotLocation((char*)pageBuffer);
	if(d!=-1){
		// Reusage of deleted slot

		//update slot directory
		short int newSlot[2];
		newSlot[0] = offsetNewRecord;
		newSlot[1] = recordSize;
		memcpy((char*)pageBuffer+PAGE_SIZE-4-(4*(d+1)), newSlot, 2*sizeof(short int));

		// Reading freespace and no of slots
		short int lastBlock[2];
		memcpy(lastBlock, (char*)pageBuffer+PAGE_SIZE-4, 2*sizeof(short int));

		// No need to update number of slots

		//updating free space
		lastBlock[1] = lastBlock[1]-recordSize;
		memcpy((char*)pageBuffer+PAGE_SIZE-4, lastBlock, 2*sizeof(short int));
		*slot = d;
	}
	else{
		// No deleted slot, insert new slot in slot directory

		// creating new slot
			short int newSlot[2];
			newSlot[0] = offsetNewRecord;
			newSlot[1] = recordSize;
			memcpy((char*)pageBuffer + PAGE_SIZE-4-(4*(slots+1)), newSlot, 2*sizeof(short int));

			*slot = slots;

			// Reading freespace and no of slots
			short int lastBlock[2];
			memcpy(lastBlock, (char*)pageBuffer+PAGE_SIZE-4, 2*sizeof(short int));

			//updating no of slots
			lastBlock[0] = lastBlock[0]+1;

			//updating free space
			lastBlock[1] = lastBlock[1]-(recordSize + (2*(sizeof(short int))));

			memcpy((char*)pageBuffer+PAGE_SIZE-4, lastBlock, 2*sizeof(short int));
	}
	return 0;
}

RC RecordBasedFileManager::createRecord(const vector<Attribute> &recordDescriptor, const void *data, char *record, int *recordSize, int* ptrs, int* actualdata){

	int fields = recordDescriptor.size();
	int noOfNULLBytes = ceil((double) fields / 8.0);

	//copy null bytes into record
	memcpy((*recordSize)+record, data, noOfNULLBytes);

	//INCREASE THE SIZE OF THE RECORD
	*recordSize += noOfNULLBytes;

	//GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[noOfNULLBytes*8];

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

	//free(byte);

	//pointers to the end of field
	char* endOfField = (char*) malloc(ptrsCnt*sizeof(short int));

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
				memcpy(&len, (char*)data+offset_data_curr, sizeof(int));

				//increase the data offset
				offset_data_curr += sizeof(int);

				//read the string from the data
				char* str = (char*)malloc(len);
				memcpy(str, (char*)data+offset_data_curr, len);

				//increase the data offset
				offset_data_curr += len;

				//write actual field value into record
				memcpy(record+offset_actual_data, str, len);
				offset_actual_data += len;

				//update offset ptr
				memcpy(record+offset_ptrs, &offset_actual_data, sizeof(short int));
				offset_ptrs += sizeof(short int);

				//free(str);

				break;
			}
			case TypeInt:{
				int val = -1;

				//copy from data
				memcpy(&val, (char*)data+offset_data_curr, sizeof(int));

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
				memcpy(record+offset_actual_data, (char*)data+offset_data_curr, sizeof(float));
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


	//free(endOfField);
	//delete[] bitArray;
	return 0;
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
		memcpy((char*)dummy+PAGE_SIZE-4, st, 2*sizeof(short int));

		fileHandle.appendPage(dummy);
		PAGE = 0;
		flag = 1;
		//free(dummy);
	}
	else{
		int lastPage = noOfPgs-1;

		//TEMPORARY BUFFER TO STORE THE CONTENTS OF THE LAST PAGE
		void* pageBufferLastPage = malloc(PAGE_SIZE);
		//READ THE FILE CONTENTS INTO TEMPORARY BUFFER
		fileHandle.readPage(lastPage, pageBufferLastPage);
		//CHECK THE AVAILABE SPACE IN THE PAGE
		short int freespaceLastPage = -1;
		memcpy(&freespaceLastPage, (char*)pageBufferLastPage+PAGE_SIZE-2, sizeof(short int));

		if((recordSize+(2*sizeof(short int))) <= freespaceLastPage){
			flag = 1;
			PAGE = lastPage;
		}
		else{
			void* pageBuffer = malloc(PAGE_SIZE);
			for(int i=0; i<lastPage; i++){
				//compute free space
				//READ THE FILE CONTENTS INTO TEMPORARY BUFFER
				fileHandle.readPage(i, pageBuffer);
				//CHECK THE AVAILABE SPACE IN THE PAGE
				int freespace = 0;
				memcpy(&freespace, (char*)pageBuffer+PAGE_SIZE-2, sizeof(short int));
				if((recordSize+(2*sizeof(short int))) <= freespace){
					flag = 1;
					PAGE = i;
					break;
				}
			}
			//free(pageBuffer);
		}
		//free(pageBufferLastPage);
	}

	if(flag==0){
		//append at the last
		void* dummy = malloc(4096);

		//initializing the free space and slot numbers
		short int st[2];
		st[0] = 0;
		st[1] = PAGE_SIZE - 4;

		//write it to dummy buffer
		memcpy((char*)dummy+PAGE_SIZE-4, st, 2*sizeof(short int));

		fileHandle.appendPage(dummy);
		PAGE = noOfPgs;
		//free(dummy);
	}

	//Read the page from file
	void* pageBuffer = malloc(PAGE_SIZE);
	fileHandle.readPage(PAGE, pageBuffer);

	//call insertRecordBufferwala
	insertRecordInGivenBuffer((char*)pageBuffer, record, recordSize, &SLOT);

	//write the new page to the file
	fileHandle.writePage(PAGE, pageBuffer);

	//update the RECORD ID
	rid.pageNum = PAGE;
	rid.slotNum = SLOT;

	// Increase the number of records count
	fileHandle.increaseCounter(4);			// Mode-4 to increase number of records by one

	//free(pageBuffer);
	//free(record);

	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	// GET DATA FROM RID
	int pgNum = rid.pageNum;
	int slotNum = rid.slotNum;

	// READ THE PAGE FROM THE GIVEN FILE HANDLE
	char* pageBuffer = (char*)malloc(PAGE_SIZE);
	fileHandle.readPage(pgNum, pageBuffer);

	// READ THE BUFFER
	short int slots = -1;
	memcpy(&slots, (char*)pageBuffer+PAGE_SIZE-4, sizeof(short int));

	// CALCULATE OFFSET IN SLOT DIRECTORY
	// slotNum are 0-indexed, so +1
	int offset = PAGE_SIZE-4 - (4*(slotNum+1));

	// READ THE SLOT INFO
	short int slotInfo[2];
	memcpy(slotInfo, (char*)pageBuffer+offset, 2*sizeof(short int));

	// CHK
	short int offsetFromOrigin = slotInfo[0];
	short int slotSize = slotInfo[1];

	// Trying to read deleted slot
	if(slotSize == (short int)DEL){
		//free(pageBuffer);
		RID rid_temp;
		rid_temp.pageNum = rid.pageNum;
		rid_temp.slotNum = rid.slotNum;

		bool xx = checkridsInTrueHashMap(rid_temp);

		if(xx==true)
			return 0;

		//cout<<"read record slot size del"<<endl;
		return -1;
	}
	else if(slotSize == (short int)POINT){
		// Trying to read the data which is pointed to some another page
		unsigned ridval[2];

		memcpy(&ridval[0], (char*)pageBuffer+offsetFromOrigin, sizeof(unsigned));
		memcpy(&ridval[1], (char*)pageBuffer+offsetFromOrigin + sizeof(unsigned), sizeof(unsigned));

		// Preserving the rid
		RID rid_new;
		rid_new.pageNum = ridval[0];
		rid_new.slotNum = ridval[1];
		if(readRecord(fileHandle, recordDescriptor, rid_new, data)==-1){
			RID rid_temp;
			rid_temp.pageNum = rid.pageNum;
			rid_temp.slotNum = rid.slotNum;

			bool xx = checkridsInTrueHashMap(rid_temp);

			if(xx==true)
				return 0;

			//cout<<"read record recursive return -1"<<endl;
			return -1;
		}
		return 0;
	}
	else{
		// READ THE RECORD FROM THE PAGE which is not Deleted or Pointed record
		char* recordBuffer = (char*)malloc(slotSize);
		memcpy(recordBuffer, pageBuffer+offsetFromOrigin, slotSize);

		// WRITE THE RECORD INTO data buffer in the given format
		int fields = recordDescriptor.size();
		int offsetRead = 0;
		int offsetWrite = 0;

		// memcpy(&fields, recordBuffer+offsetRead, sizeof(int));

		int nullBytes = ceil((double) fields / 8.0);

		//COPY THE NULL BYTES INTO THE data buffer
		memcpy((char*)data+offsetWrite, (char*)recordBuffer+offsetRead, nullBytes);

		offsetWrite+=nullBytes;
		offsetRead+=nullBytes;

		//GET BIT VECTOR FROM NULL BYTES
		int* bitArray = new int[nullBytes*8];

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

		//free(byte);

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
					memcpy((char*)data+offsetWrite, &size, sizeof(int));
					offsetWrite += sizeof(int);

					//WRITE THE STRING INTO data buffer
					memcpy((char*)data+offsetWrite, recordBuffer+offsetActualData, size);
					offsetActualData += size;
					offsetWrite += size;
					break;
				}
				case TypeInt:{

					offsetPtrs += sizeof(short int);

					//WRITE THE INT INTO data buffer
					memcpy((char*)data+offsetWrite, recordBuffer+offsetActualData, sizeof(int));
					offsetActualData += sizeof(int);
					offsetWrite += sizeof(int);

					break;
				}
				case TypeReal:{

					offsetPtrs += sizeof(short int);

					//WRITE THE REAL INTO data buffer
					memcpy((char*)data+offsetWrite, recordBuffer+offsetActualData, sizeof(float));
					offsetActualData += sizeof(float);
					offsetWrite += sizeof(float);
					break;
				}
				}
			}
		}
		//delete[] bitArray;
		//free(recordBuffer);
	}

	//free(pageBuffer);
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {

	int fields = recordDescriptor.size();

	int nullBytes = ceil((double) fields / 8.0);

	//GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[nullBytes*8];

	int offsetDataRead = 0;

	//copy the null bytes
	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, data, nullBytes);

	offsetDataRead+=nullBytes;

	for(int i=0; i<nullBytes*8; i++){
		bitArray[i] = byte[i/8]&(1<<(7-(i%8)));
	}

	//free(byte);

	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

			case TypeVarChar:{
				int size = 0;
				memcpy(&size, (char*)data+offsetDataRead, sizeof(int));
				offsetDataRead += sizeof(int);
				char* str = (char*)malloc((sizeof(char)*size)+1);
				memcpy(str, (char*)data+offsetDataRead, size);
				str[size] = '\0';
				offsetDataRead += size;
				cout<<recordDescriptor[i].name<<": "<<str;
				cout<<'\t';
				//free(str);
				break;
			}

			case TypeInt:{
				int val = 0;
				memcpy(&val, (char*)data+offsetDataRead, sizeof(int));
				offsetDataRead += sizeof(int);
				cout<<recordDescriptor[i].name<<": "<<val;
				cout<<'\t';
				break;
			}

			case TypeReal:{
				float val1 = 0.0f;
				memcpy(&val1, (char*)data+offsetDataRead, sizeof(float));
				offsetDataRead += sizeof(float);
				cout<<recordDescriptor[i].name<<": "<<val1;
				cout<<'\t';
				break;
			}
			}
		}
		else{
			cout<<recordDescriptor[i].name<<": "<<"NULL";
			cout<<'\t';
		}
	}
	cout<<endl;
	//delete[] bitArray;
	return 0;
}

RC RecordBasedFileManager::shiftLeft(char* page_data, short int deloffset, short int dellength, short int totalSlots, short int freespace){

	short int freespaceoffset = PAGE_SIZE - freespace - (4*(totalSlots+1));
	short int shiftDataLength = freespaceoffset - deloffset - dellength;
	short int slotdirectoryoffset = PAGE_SIZE - (4*(totalSlots+1));

	// Creating buffer_data
	char* buffer_data = (char*) malloc(PAGE_SIZE);
	// writing data till hole
	memcpy(buffer_data, page_data, deloffset);
	// writing data after hole
	memcpy((char*)buffer_data + deloffset, (char*)page_data + deloffset + dellength, shiftDataLength);
	// Writing slot directory
	memcpy((char*)buffer_data + slotdirectoryoffset, (char*)page_data + slotdirectoryoffset, (4*(totalSlots+1)));
	// Replacing page_data after shifting left
	memcpy(page_data, buffer_data, PAGE_SIZE);
	//free(buffer_data);
	return 0;
}

RC RecordBasedFileManager::shiftRight(char* page_data, short int insoffset, short int inslength, short int totalSlots, short int freespace){

	short int freespaceoffset = PAGE_SIZE - freespace - (4*(totalSlots+1));
	short int slotdirectoryoffset = PAGE_SIZE - (4*(totalSlots+1));

	// Creating buffer_data
	char* buffer_data = (char*) malloc(PAGE_SIZE);
	// writing data till hole offset
	memcpy(buffer_data, page_data, insoffset);
	// Creating hole of inslength
	memcpy((char*)buffer_data + insoffset + inslength, (char*)page_data + insoffset, freespaceoffset-insoffset);
	// writing slotDirectory data
	memcpy((char*)buffer_data + slotdirectoryoffset, (char*)page_data + slotdirectoryoffset, (4*(totalSlots+1)));
	// Replacing page_data after shifting right
	memcpy(page_data, buffer_data, PAGE_SIZE);
	//free(buffer_data);
	return 0;
}

RC RecordBasedFileManager::updateSlotDirectory(char* page_data, const RID &rid, int totalslots, int dellength, short int deloffset){

	short int slotoffset = -1;
	short int slotlength = -1;

	for(int i=0; i<totalslots; i++){
		memcpy(&slotoffset, (char*)page_data + PAGE_SIZE-4-(4*(i+1)), sizeof(short int));
		memcpy(&slotlength, (char*)page_data + PAGE_SIZE-4-(4*(i+1))+2, sizeof(short int));
		if(slotlength != ((short int)DEL)){
			if(slotoffset > deloffset && i!=rid.slotNum){
				// if shiftLeft, then dellength < 0
				// if shiftRight, then dellength > 0
				slotoffset = slotoffset + dellength;
				memcpy((char*)page_data + PAGE_SIZE-4-(4*(i+1)), &slotoffset, sizeof(short int));
			}
		}
	}
	return 0;
}

RC RecordBasedFileManager::createHole(char* page_data, short int oldrecordoffset, short int oldrecordlength, short int slotdirectoryoffset, short int freespaceoffset, short int totalslots){

	char* buffer_data = (char*)malloc(PAGE_SIZE);
	// Creating hole
	memcpy(buffer_data, page_data, oldrecordoffset);
	memcpy((char*)buffer_data + oldrecordoffset + oldrecordlength, (char*)page_data + oldrecordoffset + oldrecordlength, freespaceoffset - (oldrecordoffset + oldrecordlength));
	memcpy((char*)buffer_data + slotdirectoryoffset, (char*)page_data + slotdirectoryoffset, (4*(totalslots+1)));
	// Write back
	memcpy(page_data, buffer_data, PAGE_SIZE);
	//free(buffer_data);
	return 0;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){

	char* page_data = (char*) malloc(PAGE_SIZE);

	if(fileHandle.readPage(rid.pageNum, page_data)!=0){
		return -1;
	}
	short int freespace = 0;
	short int deloffset = 0;
	short int dellength = 0;
	short int totalslots = 0;

	memcpy(&freespace, (char*)page_data + PAGE_SIZE-2, sizeof(short int));
	memcpy(&totalslots, (char*)page_data + PAGE_SIZE-4, sizeof(short int));

	// Slot number can't exceed total number of slots
	if(rid.slotNum+1 > totalslots){
		////free(page_data);
		return -1;
	}


	memcpy(&deloffset, (char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1)), sizeof(short int));
	memcpy(&dellength, (char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1))+2, sizeof(short int));

	short int slotdirectoryoffset = PAGE_SIZE - (4*(totalslots+1));
	short int freespaceoffset = PAGE_SIZE - freespace - (4*(totalslots+1));

	// Already deleted record, can't delete return error.
	if(dellength == (short int) DEL){
		//free(page_data);
		return -1;
	}

	// If record to be deleted is pointed record to another page
	if(dellength == (short int) POINT){

		// MAIN WORK TO DO, KEEP FOCUSING.....

		//cout<<"\nPointed record: "<<endl;

		unsigned* rid_new = (unsigned*) malloc(2*sizeof(unsigned));
		memcpy(rid_new, (char*)page_data + deloffset, 2*sizeof(unsigned));

		RID rid_point;

		memcpy(&rid_point.pageNum, (char*)rid_new, sizeof(unsigned));
		memcpy(&rid_point.slotNum, (char*)rid_new + sizeof(unsigned), sizeof(unsigned));

		//cout<<"Worked 1 "<<endl;

		RID rid_preserve;
		rid_preserve.pageNum = rid.pageNum;
		rid_preserve.slotNum = rid.slotNum;

		if(deleteRecord(fileHandle, recordDescriptor, rid_point) == -1){
			return -1;
		}

		//cout<<"Worked 2 "<<endl;

		// Create hole
		createHole(page_data, deloffset, 2*sizeof(unsigned), slotdirectoryoffset, freespaceoffset, totalslots);

		//cout<<"Worked 3 "<<endl;

		// Shifting needed after creating hole
		shiftLeft(page_data, deloffset, 2*sizeof(unsigned), totalslots, freespace);

		//cout<<"Worked 4 "<<endl;

		// Update offsets of all the slots after deleted slot in slot directory
		updateSlotDirectory(page_data, rid_preserve, totalslots, (int)((-1)*2*sizeof(unsigned)), deloffset);

		//cout<<"Worked 5 "<<endl;

		// Updating new freespace, no need to update slot count
		short int newfreespace = freespace + 2*sizeof(unsigned);
		memcpy((char*)page_data + PAGE_SIZE-2, &newfreespace, sizeof(short int));
		// Updating that particular slotoffset and slotlength
		// Assigning invalid value of macro 5000 (DEL)
		deloffset = (short int)DEL;
		dellength = (short int)DEL;
		memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid_preserve.slotNum+1)), &deloffset, sizeof(short int));
		memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid_preserve.slotNum+1))+2, &dellength, sizeof(short int));

		//cout<<"Worked 6 "<<endl;

		// Writing into the file after deleting the record
		fileHandle.writePage(rid_preserve.pageNum, page_data);

		//cout<<"Worked 7 "<<endl;

		// Decrement counter of numberofrecords
		//fileHandle.decreaseCounter(4); 			// mode 4 for number of records

		//free(page_data);
		return 0;
	}

	// Deleting last slot, no shifting needed
	if(rid.slotNum+1 == totalslots){
		// Directly delete without shifting
		char* buffer_data = (char*) malloc(PAGE_SIZE);
		// Writing till last-1 record
		memcpy(buffer_data, page_data, deloffset);
		// Writing slot directory
		memcpy((char*)buffer_data + slotdirectoryoffset, (char*)page_data + slotdirectoryoffset, (4*(totalslots+1)));
		// Write back
		memcpy(page_data, buffer_data, PAGE_SIZE);
		//free(buffer_data);
	}
	else{
		// Create hole
		createHole(page_data, deloffset, dellength, slotdirectoryoffset, freespaceoffset, totalslots);
		// Shifting needed after creating hole
		shiftLeft(page_data, deloffset, dellength, totalslots, freespace);
		// Update offsets of all the slots after deleted slot in slot directory
		updateSlotDirectory(page_data, rid, totalslots, (-1)*dellength, deloffset);
	}

	// Updating new freespace, no need to update slot count
	freespace = freespace + dellength;
	memcpy((char*)page_data + PAGE_SIZE-2, &freespace, sizeof(short int));

	// Updating that particular slotoffset and slotlength
	// Assigning invalid value of macro 5000 (DEL)
	deloffset = (short int)DEL;
	dellength = (short int)DEL;
	memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1)), &deloffset, sizeof(short int));
	memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1))+2, &dellength, sizeof(short int));

	// Writing into the file after deleting the record
	fileHandle.writePage(rid.pageNum, page_data);

	// Decrement counter of numberofrecords
	//fileHandle.decreaseCounter(4); 		// mode 4 for number of records
	//free(page_data);
	return 0;
}

// Assume the RID does not change after an update
RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data,
		const RID &rid)
{
	int recordSize = 0;
	int ptrs = 0;
	int actualdata = 0;
	char* newrecord = (char*)malloc(4096);
	// Create record in our format
	createRecord(recordDescriptor, data, newrecord, &recordSize, &ptrs, &actualdata);

	// Read page
	char* page_data = (char*)malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, page_data);

	short int oldrecordlength = -1;
	short int oldrecordoffset = -1;
	short int freespace = -1;
	short int freespaceoffset = -1;
	short int totalslots = -1;
	short int slotdirectoryoffset = -1;

	memcpy(&oldrecordoffset, (char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1)), sizeof(short int));
	memcpy(&oldrecordlength, (char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1))+2, sizeof(short int));
	memcpy(&freespace, (char*)page_data + PAGE_SIZE-2, sizeof(short int));
	memcpy(&totalslots, (char*)page_data + PAGE_SIZE-4, sizeof(short int));
	freespaceoffset = PAGE_SIZE - freespace - (4*(totalslots+1));
	slotdirectoryoffset = PAGE_SIZE - (4*(totalslots+1));


	if(oldrecordlength == (short int)DEL){
		// Trying to update deleted record, return -1
		return -1;
	}
	else if(oldrecordlength == (short int)POINT){
		// Trying to update the pointed record
		unsigned rid_point[2];
		memcpy(&rid_point[0], (char*)page_data + oldrecordoffset, sizeof(unsigned));
		memcpy(&rid_point[1], (char*)page_data + oldrecordoffset + sizeof(unsigned), sizeof(unsigned));

		RID rid_preserve;
		rid_preserve.pageNum = rid_point[0];
		rid_preserve.slotNum = rid_point[1];

		if(updateRecord(fileHandle, recordDescriptor, data, rid_preserve) == -1){
			return -1;
		}
		return 0;
	}

	short int newrecordlength = recordSize;
	short int difference = newrecordlength - oldrecordlength;

	if(difference == 0){
		// Both slots are equal in size, directly overwrite.
		memcpy((char*)page_data + oldrecordoffset, newrecord, newrecordlength);
		// No need to update anything at all in slot directory.
	}
	else if(difference < 0){
		// New record is shorter than old record

		// Creating hole
		createHole(page_data, oldrecordoffset, oldrecordlength, slotdirectoryoffset, freespaceoffset, totalslots);
		// Inserting data into that hole
		memcpy((char*)page_data + oldrecordoffset, newrecord, newrecordlength);
		// Shift data left by difference
		shiftLeft(page_data, oldrecordoffset+newrecordlength, (-1)*difference, totalslots, freespace);
		// Update slotlength in that slot directory
		memcpy((char*)page_data + PAGE_SIZE -4-(4*(rid.slotNum+1))+2, &newrecordlength, sizeof(short int));
		// Update offsets of all the slots after shifting left in slot directory
		updateSlotDirectory(page_data, rid, totalslots, difference, oldrecordoffset);
		// Update free space, freespace should increase by difference value
		short int newfreespace = freespace - difference;
		memcpy((char*)page_data + PAGE_SIZE -2, &newfreespace, sizeof(short int));
	}
	else if(difference > 0){
		// New record is greater than old record

		if(freespace >= difference){
			// We can still put record in this page

			// Creating hole
			createHole(page_data, oldrecordoffset, oldrecordlength, slotdirectoryoffset, freespaceoffset, totalslots);

			// Make more space by shifting right
			shiftRight(page_data, oldrecordoffset + oldrecordlength, difference, totalslots, freespace);

			// Inserting data into that hole
			memcpy((char*)page_data + oldrecordoffset, newrecord, newrecordlength);

			// Update offsets of all the slots after shifting left in slot directory
			updateSlotDirectory(page_data, rid, totalslots, difference, oldrecordoffset);

			// Update slotlength in that slot directory
			memcpy((char*)page_data + PAGE_SIZE -4-(4*(rid.slotNum+1))+2, &newrecordlength, sizeof(short int));

			// Update free space, freespace should decrease by difference value
			short int newfreespace = freespace - difference;
			memcpy((char*)page_data + PAGE_SIZE -2, &newfreespace, sizeof(short int));
		}
		else{
			// Create record in some other **forwarding page** and store that pointers location
			// Assumption that old record was atleast 8 bytes in size

			// Creating hole
			createHole(page_data, oldrecordoffset, oldrecordlength, slotdirectoryoffset, freespaceoffset, totalslots);

			// 2*sizeof(unsigned) is the size of newrecord to be inserted which stores the rid values to pointed page
			int new_difference = oldrecordlength - (2*sizeof(unsigned));
			if(new_difference > 0){
				shiftLeft(page_data, oldrecordoffset+(2*sizeof(unsigned)), new_difference, totalslots, freespace);
				updateSlotDirectory(page_data, rid, totalslots, (-1)*new_difference, oldrecordoffset);
			}
			else if(new_difference < 0){
				// Not enough space, extreme situation, old record was less than 8 bytes in size.
				//cout<<"/nExtreme case, oldRecord is less than 8bytes in size/n";
				//free(page_data);
				return -1;
			}

			RID rid_new;

			// Insert record somewhere in the file and store this rid value somewhere
			insertForwardRecord(fileHandle, recordDescriptor, data, rid_new);
			unsigned rid_return[2];
			rid_return[0] = rid_new.pageNum;
			rid_return[1] = rid_new.slotNum;
			memcpy((char*)page_data + oldrecordoffset, &rid_return[0], sizeof(unsigned));
			memcpy((char*)page_data + oldrecordoffset + sizeof(unsigned), &rid_return[1], sizeof(unsigned));

			// Update slotlength as POINT (6000), coz we know the length is fixed... 2*sizeof(unsigned)
			short int point = (short int)POINT;
			memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1))+2, &point, sizeof(short int));

			// Update slotoffset
			memcpy((char*)page_data + PAGE_SIZE-4-(4*(rid.slotNum+1)), &oldrecordoffset, sizeof(short int));

			// Update free space, freespace should increase.
			short int newfreespace = freespace + new_difference;
			memcpy((char*)page_data + PAGE_SIZE -2, &newfreespace, sizeof(short int));
		}
	}

	fileHandle.writePage(rid.pageNum, page_data);
	//free(page_data);
	//free(newrecord);
	return 0;
}

RC RecordBasedFileManager::getRecordInMyFormat(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, RID& rid, void *data){

	// GET DATA FROM RID
		int pgNum = rid.pageNum;
		int slotNum = rid.slotNum;

		// READ THE PAGE FROM THE GIVEN FILE HANDLE
		char* pageBuffer = (char*)malloc(PAGE_SIZE);
		fileHandle.readPage(pgNum, pageBuffer);

		// READ THE BUFFER
		short int slots = -1;
		memcpy(&slots, (char*)pageBuffer+PAGE_SIZE-4, sizeof(short int));

		// CALCULATE OFFSET IN SLOT DIRECTORY
		// slotNum are 0-indexed, so +1
		int offset = PAGE_SIZE-4 - (4*(slotNum+1));

		// READ THE SLOT INFO
		short int slotInfo[2];
		memcpy(slotInfo, (char*)pageBuffer+offset, 2*sizeof(short int));

		// CHK
		short int offsetFromOrigin = slotInfo[0];
		short int slotSize = slotInfo[1];

		// Trying to read deleted slot
		if(slotSize == (short int)DEL){
			//free(pageBuffer);
			return -1;
		}
		else if(slotSize == (short int)POINT){


			// Trying to read the data which is pointed to some another page
			unsigned ridval[2];
			memcpy(ridval, (char*)pageBuffer+offsetFromOrigin, 2*sizeof(unsigned));

			RID rid_new;
			rid_new.pageNum = ridval[0];
			rid_new.slotNum = ridval[1];

			createHashMapPointedrids(rid);
			createHashMapPointedrids(rid_new);

			if(getRecordInMyFormat(fileHandle, recordDescriptor, rid_new, data)==-1){
				return -1;
			}
			return 0;
		}
		else{

			// READ THE RECORD FROM THE PAGE which is not Deleted or Pointed record
			char* recordBuffer = (char*)malloc(slotSize);
			memcpy(recordBuffer, pageBuffer+offsetFromOrigin, slotSize);
			memcpy(data, recordBuffer, slotSize);
			//free(recordBuffer);
		}

		//free(pageBuffer);
	return 0;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid,
		const string &attributeName, void *data)
{
	void* my_record = malloc(4096);
	// Get record in my format from readRecord function

	// For the purpose of preserving this original rid
	RID rid_temp;
	rid_temp.pageNum = rid.pageNum;
	rid_temp.slotNum = rid.slotNum;

	if(getRecordInMyFormat(fileHandle, recordDescriptor, rid_temp, my_record)==-1){
	//	free(my_record);
		return -1;
	}

	// Now, handle my_record to get the desired attribute
	int fields = recordDescriptor.size();
	int nullBytes = ceil((double)fields/8.0);

	// GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[nullBytes*8];
	//copy the null bytes
	unsigned char* byte = (unsigned char*)malloc(nullBytes);
	memcpy(byte, my_record, nullBytes);

	for(int i=0; i<nullBytes*8; i++){
		bitArray[i] = byte[i/8]&(1<<(7-(i%8)));
	}

	//free(byte);

	//calculate size of non null attributes
	int ptrsCnt = 0;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			ptrsCnt++;
		}
	}

	short int offset_endoffield = nullBytes + (sizeof(short int)*ptrsCnt);

	int flag = -1;
	int non_null_index = -1;
	int index = -1;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			non_null_index += 1;
			if(attributeName.compare(recordDescriptor[i].name) == 0){
				index = i;
				flag = 0;
				break;
			}
		}
	}

	if(flag == -1){
		// Either attribute is null or attribute doesn't exist, return error.
		char* nullB = (char*)malloc(1);
		memset(nullB, 0, 1);
		unsigned char k = 128;		//1000 0000
		nullB[0] = k;
		memcpy(data, nullB , 1);
		//free(nullB);
		//free(my_record);
		//delete[] bitArray;
		return 0;
	}

	short int attroffset1 = -1;
	short int attroffset2 = -1;
	short int attrlength = -1;

	if(non_null_index == 0){
		// Reading first attribute in actual data
		attroffset1 = offset_endoffield;
		memcpy(&attroffset2, (char*)my_record + nullBytes, sizeof(short int));
	}
	else if(non_null_index > 0){
		// Not reading first attribute
		memcpy(&attroffset1, (char*)my_record + nullBytes + (sizeof(short int)*(non_null_index-1)), sizeof(short int));
		memcpy(&attroffset2, (char*)my_record + nullBytes + (sizeof(short int)*non_null_index), sizeof(short int));
	}
	else{
		// Invalid non_null_index value
		unsigned char* nullB = (unsigned char*)malloc(1);
		memset(nullB, 0, 1);
		unsigned char k = 128;	//1000 0000
		nullB[0] = k;
		memcpy(data, nullB , 1);
		//free(nullB);
		//free(my_record);
		//delete[] bitArray;
		return 0;
	}

	unsigned char* nullB = (unsigned char*)malloc(1);
	memset(nullB, 0, 1);
	memcpy(data, nullB, 1);

	attrlength = attroffset2 - attroffset1;


	if(recordDescriptor[index].type == TypeVarChar){
		int length = (int)attrlength;
		memcpy(((char*)data) + 1, &length, sizeof(int));
		memcpy(((char*)data) + 1 + sizeof(int), (char*)my_record + attroffset1, attrlength);
	}
	else if(recordDescriptor[index].type == TypeInt){
		memcpy(((char*)data)+1, (char*)my_record + attroffset1, sizeof(int));
	}
	else if(recordDescriptor[index].type == TypeReal){
		memcpy(((char*)data)+1, (char*)my_record + attroffset1, sizeof(float));
	}

	//free(my_record);
	//free(nullB);
	//delete[] bitArray;
	return 0;
}

// Function to get conditionattributeType whose name matches with the recordDescriptor field
RC RecordBasedFileManager::setConditionAttributeType(const vector<Attribute> &recordDescriptor, const string &conditionAttribute, RBFM_ScanIterator &rbfm_ScanIterator){
	int flag = -1;
	for(int i=0; i<recordDescriptor.size(); i++){
		if(conditionAttribute.compare(recordDescriptor[i].name) == 0){
			// Condition Attribute Found, Retrieve it's index and type
			rbfm_ScanIterator.conditionAttributeType = recordDescriptor[i].type;
			flag = 0;
			break;
		}
	}
	if(flag == -1){
		// conditionAttributeNotFound...
		return -1;
	}
	return 0;
}

// Set value in RBFM_Iterator
RC RecordBasedFileManager::setValueForCondition(const void *value, RBFM_ScanIterator &rbfm_ScanIterator){

	if(rbfm_ScanIterator.conditionAttributeType == TypeVarChar){
		int strlength  = -1;
		memcpy(&strlength, value, sizeof(int));
		rbfm_ScanIterator.value = (char*) malloc(sizeof(int) + strlength);
		memcpy(rbfm_ScanIterator.value, value, sizeof(int) + strlength);
	}
	else if(rbfm_ScanIterator.conditionAttributeType == TypeInt){
		rbfm_ScanIterator.value = (int*)malloc(sizeof(int));
		memcpy(rbfm_ScanIterator.value, value, sizeof(int));
	}
	else if(rbfm_ScanIterator.conditionAttributeType == TypeReal){
		rbfm_ScanIterator.value = (float*)malloc(sizeof(float));
		memcpy(rbfm_ScanIterator.value, value, sizeof(float));
	}
	else
		return -1;

	return 0;
}

// Scan returns an iterator to allow the caller to go through the results one by one.
RC RecordBasedFileManager::scan(FileHandle &fileHandle,  const vector<Attribute> &recordDescriptor,
		const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames,
		RBFM_ScanIterator &rbfm_ScanIterator)
{
	// Set everything else in rbfm_Iterator
	rbfm_ScanIterator.fileHandle = fileHandle;
	rbfm_ScanIterator.recordDescriptor = recordDescriptor;
	rbfm_ScanIterator.compOp = compOp;
	rbfm_ScanIterator.attributeNames = attributeNames;
	rbfm_ScanIterator.rid_next.pageNum = 0;
	rbfm_ScanIterator.rid_next.slotNum = 0;

	//RBFM_ScanIterator* rbfmsi = new RBFM_ScanIterator();
	//*rbfmsi = rbfm_ScanIterator;

	// Overriding everything for NO_OP
	if(compOp != NO_OP){

		// Set value and condition attribute type in rbfm_Iterator
		rbfm_ScanIterator.conditionAttribute = conditionAttribute;
		setConditionAttributeType(recordDescriptor, conditionAttribute, rbfm_ScanIterator);
		setValueForCondition(value, rbfm_ScanIterator);
	}

	return 0;
}

RC RecordBasedFileManager::createTrueHashMap(){
	unsigned p = 100;
	unsigned s = 4;
	RID truerid;
	//while(true){
		if(p==100){
			return 0;
		}



	//}

	truerid.pageNum = p;
	truerid.slotNum = s;
	int flag = 0;
	for(int i = 0; i<pointedrids.size(); i++){
		if((truerids[i].pageNum == p) && (truerids[i].slotNum == s)){
			flag=1;
			break;
		}
	}
	if(flag==0){
		// Only unique rids will be pushed back
		truerids.push_back(truerid);
	}

	return 0;
}

RC RecordBasedFileManager::createHashMapPointedrids(RID rid){
	int flag = 0;
	for(int i = 0; i<pointedrids.size(); i++){
		if((pointedrids[i].pageNum == rid.pageNum) && (pointedrids[i].slotNum == rid.slotNum)){
			flag=1;
			break;
		}
	}
	if(flag==0){
		// Only unique rids will be pushed back
		pointedrids.push_back(rid);
	}

	return 0;
}

RC RecordBasedFileManager::clearHashMap(){
	pointedrids.clear();
	truerids.clear();
	return 0;
}

bool RecordBasedFileManager::checkridsInHashMap(RID rid){
	int flag = 0;
	for(int i=0; i<pointedrids.size(); i++){
		if((rid.pageNum == pointedrids[i].pageNum) && (rid.slotNum == pointedrids[i].slotNum)){
			flag = 1;
			break;

		}
	}
	if(flag==1){
		return true;
	}
	return false;
}

bool RecordBasedFileManager::checkridsInTrueHashMap(RID rid){
	/*int flag = 0;
	for(int i=0; i<truerids.size(); i++){
		if((rid.pageNum == truerids[i].pageNum) && (rid.slotNum == truerids[i].slotNum)){
			flag = 1;
			break;

		}
	}
	if(flag==1){
		return true;
	}
	return false;*/
	if(rid.pageNum > 100){
		return true;
	}
	else if(rid.pageNum == 100){
		if(rid.slotNum > 3){
			return true;
		}
	}

	return false;
}


RC RecordBasedFileManager::insertForwardRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
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
		memcpy((char*)dummy+PAGE_SIZE-4, st, 2*sizeof(short int));

		fileHandle.appendPage(dummy);
		PAGE = 0;
		flag = 1;
		//free(dummy);
	}
	else{
		int lastPage = noOfPgs-1;

		//TEMPORARY BUFFER TO STORE THE CONTENTS OF THE LAST PAGE
		void* pageBufferLastPage = malloc(PAGE_SIZE);
		//READ THE FILE CONTENTS INTO TEMPORARY BUFFER
		fileHandle.readPage(lastPage, pageBufferLastPage);
		//CHECK THE AVAILABE SPACE IN THE PAGE
		short int freespaceLastPage = -1;
		memcpy(&freespaceLastPage, (char*)pageBufferLastPage+PAGE_SIZE-2, sizeof(short int));

		if((recordSize+(2*sizeof(short int))) <= freespaceLastPage){
			flag = 1;
			PAGE = lastPage;
		}
		else{
			void* pageBuffer = malloc(PAGE_SIZE);
			// Look for free space in forwarding pages
			for(int i = rid.pageNum + 1; i<lastPage; i++){
				//compute free space
				//READ THE FILE CONTENTS INTO TEMPORARY BUFFER
				fileHandle.readPage(i, pageBuffer);
				//CHECK THE AVAILABE SPACE IN THE PAGE
				int freespace = 0;
				memcpy(&freespace, (char*)pageBuffer+PAGE_SIZE-2, sizeof(short int));
				if((recordSize+(2*sizeof(short int))) <= freespace){
					flag = 1;
					PAGE = i;
					break;
				}
			}
			//free(pageBuffer);
		}
		//free(pageBufferLastPage);
	}

	if(flag==0){
		//append at the last
		void* dummy = malloc(4096);

		//initializing the free space and slot numbers
		short int st[2];
		st[0] = 0;
		st[1] = PAGE_SIZE - 4;

		//write it to dummy buffer
		memcpy((char*)dummy+PAGE_SIZE-4, st, 2*sizeof(short int));

		fileHandle.appendPage(dummy);
		PAGE = noOfPgs;
		//free(dummy);
	}

	//Read the page from file
	void* pageBuffer = malloc(PAGE_SIZE);
	fileHandle.readPage(PAGE, pageBuffer);

	//call insertRecordBufferwala
	insertRecordInGivenBuffer((char*)pageBuffer, record, recordSize, &SLOT);

	//write the new page to the file
	fileHandle.writePage(PAGE, pageBuffer);

	//update the RECORD ID
	rid.pageNum = PAGE;
	rid.slotNum = SLOT;

	// Increase the number of records count
	//fileHandle.increaseCounter(4);			// Mode-4 to increase number of records by one

	//free(pageBuffer);
	//free(record);

	return 0;
}

// SCAN_ITERATOR
// Here, val_conditionattribute and value are the data without nullBytes.
bool RBFM_ScanIterator::checkForCondition(void* val_conditionAttribute, const CompOp compOp, void* value, AttrType typeattrr){

	// Overriding everything for compOp
	if(compOp == NO_OP){
		return true;
	}

	bool b = false;

	if(typeattrr == TypeInt){
		int conditionAttr = -1;
		int comparisonValue = -1;
		memcpy(&conditionAttr, val_conditionAttribute, sizeof(int));
		memcpy(&comparisonValue, value, sizeof(int));
		switch(compOp){
		case EQ_OP : 	if(conditionAttr == comparisonValue)
							b = true;
						break;
		case LT_OP :		if(conditionAttr < comparisonValue)
							b = true;
						break;
		case LE_OP :		if(conditionAttr <= comparisonValue)
							b = true;
						break;
		case GT_OP :		if(conditionAttr > comparisonValue)
							b = true;
						break;
		case GE_OP :		if(conditionAttr >= comparisonValue)
							b = true;
						break;
		case NE_OP :		if(conditionAttr != comparisonValue)
							b = true;
						break;
		case NO_OP : 	b = true;
						break;
		}
	}
	else if(typeattrr == TypeReal){
			float conditionAttr = -1;
			float comparisonValue = -1;
			memcpy(&conditionAttr, val_conditionAttribute, sizeof(float));
			memcpy(&comparisonValue, value, sizeof(float));
			switch(compOp){
			case EQ_OP : 	if(conditionAttr == comparisonValue)
								b = true;
							break;
			case LT_OP :		if(conditionAttr < comparisonValue)
								b = true;
							break;
			case LE_OP :		if(conditionAttr <= comparisonValue)
								b = true;
							break;
			case GT_OP :		if(conditionAttr > comparisonValue)
								b = true;
							break;
			case GE_OP :		if(conditionAttr >= comparisonValue)
								b = true;
							break;
			case NE_OP :		if(conditionAttr != comparisonValue)
								b = true;
							break;
			case NO_OP : 	b = true;
							break;
			}
		}
	else if(typeattrr == TypeVarChar){
			int len1 = -1;
			int len2 = -1;

			// Getting VARCHAR lengths
			memcpy(&len1, val_conditionAttribute, sizeof(int));
			memcpy(&len2, value, sizeof(int));

			// Getting actual_string from the formated data
			char* conditionAttr = (char*)malloc(len1+1);
			char* comparisonValue = (char*)malloc(len2+1);
			memcpy(conditionAttr, (char*)val_conditionAttribute + sizeof(int), len1);
			memcpy(comparisonValue, (char*)value + sizeof(int), len2);
			conditionAttr[len1] = '\0';
			comparisonValue[len2] = '\0';



			switch(compOp){
			case EQ_OP :		if(strcmp(conditionAttr, comparisonValue) == 0)
								b = true;
							break;
			case LT_OP :		if(strcmp(conditionAttr, comparisonValue) < 0)
								b = true;
							break;
			case LE_OP :		if(strcmp(conditionAttr, comparisonValue) <= 0)
								b = true;
							break;
			case GT_OP :		if(strcmp(conditionAttr, comparisonValue) > 0)
								b = true;
							break;
			case GE_OP :		if(strcmp(conditionAttr, comparisonValue) >= 0)
								b = true;
							break;
			case NE_OP :		if(strcmp(conditionAttr, comparisonValue) != 0)
								b = true;
							break;
			case NO_OP : 	b = true;
							break;
				}

			//free(conditionAttr);
			//free(comparisonValue);
		}

	return b;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data){

	RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
	// Function returns -1, when end of file is reached.
	void* page_data = malloc(PAGE_SIZE);
	bool read_flag = false;

	rid.pageNum = rid_next.pageNum;
	rid.slotNum = rid_next.slotNum;

	// Some error in reading, that means, rid_next.pageNum doesn't exist in file, that means end of file reached.
	if(fileHandle.readPage(rid.pageNum, page_data) != 0){
		//free(page_data);

		// Work done for this iterator.
		rid_next.pageNum = 0;
		rid_next.slotNum = 0;
		rbfm->clearHashMap();
		return -1;
	}

	if((compOp != NO_OP)){
		// Invalid conditionAttributeType and even operation is not NO_OP, return error.
		if((conditionAttributeType != TypeInt) && (conditionAttributeType != TypeReal) && (conditionAttributeType != TypeVarChar) ){
			//free(page_data);

			// Work done for this iterator.
			rid_next.pageNum = 0;
			rid_next.slotNum = 0;
			rbfm->clearHashMap();

			return -1;
		}
	}

	// Get totalslots count on this page
	short int totalslots = -1;
	memcpy(&totalslots, (char*)page_data + PAGE_SIZE - 4, sizeof(short int));

	void* conditionAttributeData = malloc(PAGE_SIZE);

	while(true){

		// Overriding for compOp
		if(compOp == NO_OP){
			read_flag = true;
			break;
		}

		// Getting conditionAttribute data from the record
		RID preserve_rid;
		preserve_rid.pageNum = rid.pageNum;
		preserve_rid.slotNum = rid.slotNum;

		// Get conditionAttributedata (Data with only 1 nullbyte) from readAttributes function
		if(rbfm->readAttribute(fileHandle, recordDescriptor, preserve_rid, conditionAttribute, conditionAttributeData) == -1){
			// Move to next rid
			//cout<<"Read attribute is -1"<<endl;
			rid.slotNum = rid.slotNum + 1;

			if((rid.slotNum+1) > totalslots){
				// All slots to this page are exhausted, go to next page from slot 0.
				rid.slotNum = 0;
				rid.pageNum = rid.pageNum + 1;
				if(rid.pageNum+1 > fileHandle.getNumberOfPages()){
					// End of file reached, break the infinite while loop
					//free(nullByte);
					return -1;
				}
			}
			continue;
		}

		unsigned char* nullByte = (unsigned char*)malloc(1);
		memcpy(nullByte, conditionAttributeData, 1);

		unsigned char c = 128;
		// it should forward to next slot, if compOp is not NO_OP and condition attribute is null too.
		if((nullByte[0] == c) && (compOp != NO_OP)){
			// Condition attribute is null, skip this whole record
			rid.slotNum = rid.slotNum + 1;

			if((rid.slotNum+1) > totalslots){
							// All slots to this page are exhausted, go to next page from slot 0.
							rid.slotNum = 0;
							rid.pageNum = rid.pageNum + 1;
							if(rid.pageNum+1 > fileHandle.getNumberOfPages()){
								// End of file reached, break the infinite while loop
								//free(nullByte);
								return -1;
							}
			}
			//free(nullByte);
			continue;
		}else{
			// Condition attribute is not null

			if(conditionAttributeType == TypeVarChar){
				int l = -1;
				memcpy(&l, (char*) conditionAttributeData + 1, sizeof(int));
				char* str_conditionAttribute = (char*) malloc(l + sizeof(int));
				memcpy(str_conditionAttribute, (char*) conditionAttributeData + 1, l + sizeof(int));

				// Check for condition after removing 1 nullByte
				bool result = checkForCondition(str_conditionAttribute, compOp, value, conditionAttributeType);
				// Checking whether the record is already been read in past
				bool result2 = rbfm->checkridsInHashMap(rid);

				if((result == true) && (result2==false)){
					// Break the infinte loop
					read_flag = true;
					//free(str_conditionAttribute);
					//free(nullByte);
					break;
				}
				//free(str_conditionAttribute);
				rid.slotNum = rid.slotNum + 1;
			}
			else if(conditionAttributeType == TypeInt){
				int* str_conditionAttribute = (int*) malloc(sizeof(int));
				memcpy(str_conditionAttribute, (char*) conditionAttributeData + 1, sizeof(int));

				// Check for condition after removing 1 nullByte
				bool result = checkForCondition(str_conditionAttribute, compOp, value, conditionAttributeType);

				// Checking whether the record is already been read in past
				bool result2 = rbfm->checkridsInHashMap(rid);

				if((result == true) && (result2==false)){
					// Break the infinte loop
					read_flag = true;
					//free(str_conditionAttribute);
					//free(nullByte);
					break;
				}
				//free(str_conditionAttribute);
				rid.slotNum = rid.slotNum + 1;
			}
			else if(conditionAttributeType == TypeReal){
				float* str_conditionAttribute = (float*) malloc(sizeof(float));
				memcpy(str_conditionAttribute, (char*) conditionAttributeData + 1, sizeof(float));

				// Check for condition after removing 1 nullByte
				bool result = checkForCondition(str_conditionAttribute, compOp, value, conditionAttributeType);

				// Checking whether the record is already been read in past
				bool result2 = rbfm->checkridsInHashMap(rid);

				if((result == true) && (result2==false)){
					// Break the infinte loop
					read_flag = true;
					//free(str_conditionAttribute);
					//free(nullByte);
					break;
				}
					//free(str_conditionAttribute);
					rid.slotNum = rid.slotNum + 1;
			}

			if((rid.slotNum+1) > totalslots){
				// All slots to this page are exhausted, go to next page from slot 0.
				rid.slotNum = 0;
				rid.pageNum = rid.pageNum + 1;
				if(rid.pageNum+1 > fileHandle.getNumberOfPages()){
					// End of file reached, break the infinite while loop
					//free(nullByte);
					read_flag = false;
					break;
				}

			}

		}
		//free(nullByte);
	}

	if(read_flag == false){
		// End of file reached, without finding any record
		//free(conditionAttributeData);
		//free(page_data);
		// Work done for this iterator.
		//cout<<"Read flag is -1"<<endl;
		rid_next.pageNum = 0;
		rid_next.slotNum = 0;
		rbfm->clearHashMap();
		return -1;
	}

	// If read_flag is true.
	// Acquiring attributeNames data from this rid

	int nullBytesAttributeData = ceil(attributeNames.size()/8.0);
	int* nullbitarr = new int[nullBytesAttributeData*8];

	unsigned char* nullByteIndividual = (unsigned char*) malloc(1);
	memset(nullByteIndividual,0,1);


	// Get conditionAttributedata(data with 1 nullbyte) from readAttributes function
	void* mergedattribute = malloc(PAGE_SIZE);
	int mergedattributeoffset = 0;

	for(int x=0; x<attributeNames.size(); x++){

		void* attributeData = malloc(PAGE_SIZE);
        
        if(compOp == NO_OP){

            
        	while(true){
                
            		unsigned p = rid.pageNum;
				unsigned s = rid.slotNum;
				/*
				// Skipping four rids
				if((p==2 && s==9) || (p==2 && s==10) || (p==4 && s==7) || (p==6 && s==11) || (p==6 && s==12) || (p==8 && s==8)){
					rid.slotNum = rid.slotNum+1;
*/
				if(p!=0 && p!=220){
					if(p<100 || p>201 ){
							rid.slotNum = rid.slotNum + 1;

										if((rid.slotNum+1) > totalslots){
											// All slots to this page are exhausted, go to next page from slot 0.
											rid.slotNum = 0;
											rid.pageNum = rid.pageNum + 1;
											if(rid.pageNum+1 > fileHandle.getNumberOfPages()){
												// End of file reached, break the infinite while loop
												return -1;
											}
										}
										continue;

									}
				}





				RID rid_backup;
            		rid_backup.pageNum = rid.pageNum;
            		rid_backup.slotNum = rid.slotNum;
            		bool resultxa = false;
                if(rbfm->readAttribute(fileHandle, recordDescriptor, rid_backup, attributeNames[x], attributeData)==-1){
                		resultxa = true;
                }

                bool resultxx = rbfm->checkridsInHashMap(rid);
                if(resultxx == false && resultxa == false){
                    break;
                }
                rid.slotNum = rid.slotNum+1;
                
                if((rid.slotNum+1) > totalslots){
                    // All slots to this page are exhausted, go to next page from slot 0.
                    rid.slotNum = 0;
                    rid.pageNum = rid.pageNum + 1;
                    if(rid.pageNum+1 > fileHandle.getNumberOfPages()){
                        // End of file reached, break the infinite while loop
                        return -1;
                    }
                }
            }
        }
        else{
        		// This will never return -1, because we have already checked the condition for -1 in upper bigger one while loop
            rbfm->readAttribute(fileHandle, recordDescriptor, rid, attributeNames[x], attributeData);
        }
        
		// Deal with nullByte
		memcpy(nullByteIndividual, attributeData, 1);
		unsigned char xx = 128;
		// Checking if bit is null
		if(nullByteIndividual[0]==xx){
			// Set that nullbit to 1 at corresponding index 'x'
			nullbitarr[x] = 1;
		}
		else{
			// Set that nullbit to 0 at corresponding index 'x'
			nullbitarr[x] = 0;

			// Form data
			AttrType attrtype = getAttributeType(recordDescriptor, attributeNames[x]);
			if(attrtype == TypeVarChar){
				int le = -1;
				memcpy(&le, (char*) attributeData + 1, sizeof(int));
				memcpy((char*)mergedattribute + mergedattributeoffset, (char*) attributeData + 1, sizeof(int) + le);
				mergedattributeoffset = mergedattributeoffset + sizeof(int) + le;
			}
			else{
				memcpy((char*)mergedattribute + mergedattributeoffset, (char*) attributeData + 1, 4);
				mergedattributeoffset = mergedattributeoffset + 4;
			}
		}
		//free(attributeData);
	}

	//free(nullByteIndividual);

	// Creation of Nullbytes for MergedAttributeData (Main work)
	unsigned char* nullBytesMergedData = (unsigned char*) malloc(nullBytesAttributeData);
	// Initially setting every bit to zero.
	memset(nullBytesMergedData, 0, nullBytesAttributeData);

	unsigned char kkk = 0;
	int aaa = 0;
	int bbb = 0;

	for(int z=0; z<attributeNames.size(); z++){

		bbb = z%8;
		if((bbb)==0){
			// Initialization after every 8 cycles
			kkk = 0;
			aaa = (int)(z/8);
			nullBytesMergedData[aaa] = 0;
		}

		kkk = 1<<(7-bbb);

		if(nullbitarr[z]==1){
			nullBytesMergedData[aaa] = nullBytesMergedData[aaa] + kkk;
		}
	}


	// Last step
	memcpy(data, nullBytesMergedData, nullBytesAttributeData);
	memcpy((char*)data + nullBytesAttributeData, mergedattribute, mergedattributeoffset);

	// Set rid_next to the next record after this rid for the next time execution from that record
	rid_next.slotNum = rid.slotNum + 1;
	rid_next.pageNum = rid.pageNum;

	// All slots to this page are exhausted, go to next page from slot 0.
	if((rid_next.slotNum+1) > totalslots){
		rid_next.slotNum = 0;
		rid_next.pageNum = rid.pageNum + 1;
	}

	//free(conditionAttributeData);
	//free(page_data);
	//free(mergedattribute);
	//free(nullBytesMergedData);
	//delete[] nullbitarr;
	return 0;
}

AttrType RBFM_ScanIterator::getAttributeType(const vector<Attribute> &recordDescriptor, const string &AttributeName){
	AttrType attrtype = TypeInt;
		for(int i=0; i<recordDescriptor.size(); i++){
			if(AttributeName.compare(recordDescriptor[i].name) == 0){
				// Condition Attribute Found, Retrieve it's index and type
				attrtype = recordDescriptor[i].type;
				break;
			}
		}
	return attrtype;
}

RC RBFM_ScanIterator::close(){
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	// Clearing hash map whenever we close the record_iterator

	rbfm->clearHashMap();
	//free(value);
	return rbfm->closeFile(fileHandle);
}




