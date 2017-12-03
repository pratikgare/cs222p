#include "rbfm.h"

#include <stdlib.h>
#include <cmath>
#include <cstdio>
#include<string>
#include <cstring>
#include <iostream>
#include <new>


// My created functions
RC setTotalSlots(void* data, short &value);
RC getTotalSlots(void* data, short &value);
RC incrementTotalSlots(void* data);
//RC decrementTotalSlots(void* data);
int setFreeSpace(void* data, short &value);
RC rbf_getFreeSpace(void* data, short &value);
RC incrementFreeSpace(void* data, int &value);
RC decrementFreeSpace(void* data, int &value);
RC getSlotOffset(void* data, int &slotNum, short &slotOffset);
RC getSlotLength(void* data, int &slotNum, short &slotLength);
RC setSlotOffset(void* data, int &slotNum, int &slotOffset);
RC setSlotLength(void* data, int &slotNum, int &slotLength);
RC initializeNewPage(void* data);
RC deletedSlotLocation(void* data);
RC extractNullbitArray(const void* data, int* nullbitArr, int &fields);
RC extractFieldData(const void* data, int &offset, AttrType type, void* fieldData, int &fieldDataSize);
RC createRecordInMyFormat(const void *data, void* record, int &recordSize, const vector<Attribute> &recordDescriptor);
RC updateRightSlotDirectory(void* data, int &slotNum, int &shiftRightLength);
RC updateLeftSlotDirectory(void* data, int &slotNum, int &shiftLeftLength);
RC freeSpaceFinder(FileHandle &fileHandle, int &recordSize, void* page, int &startPageNum, int &lastPageNum);
RC createHoleRecord(void* data, short &offset, short &holeLength);
RC shiftLeftRecord(void* data, short &offset, short &shiftLength);
RC shiftRightRecord(void* data, short &offset, short &shiftLength);
RC insertInGivenPage(void* page, void* record, int &recordSize, RID &rid);
RC getDataFromMyRecordFormat(void *pageData, void* data, const RID &rid, const vector<Attribute> &recordDescriptor);
RC readForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, RID &rid_new, void *data);
RC deleteForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, RID &rid_new);
RC insertForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, void* record, int &recordSize, const RID &rid, RID &rid_new);
RC updateForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, void *record, int &recordSize, RID &rid_new);
RC getDataInMyFormat(void *pageData, void* record, int &recordSize, const RID &rid);
RC readMyFormatForwarderRecord(FileHandle &fileHandle, RID &rid_new, void *record, int &recordSize);
RC extractSingleAttributeFromMyData(void* record, int &recordSize, const vector<Attribute> &recordDescriptor, const string &attributeName, void* data, int &dataSize);
RC extractConditionAttributeType(AttrType &type, const vector<Attribute> &recordDescriptor, const string &attributeName);



// My Defined Values - Any value greater than PAGE_SIZE
#define DEL 5000
#define POINT 6000

RC setTotalSlots(void* data, short &value){
	// 2nd last (short) slot from the end
	int offset = PAGE_SIZE - (2*sizeof(short));
	memcpy((char*)data + offset, &value, sizeof(short));
	return 0;
}

RC getTotalSlots(void* data, short &value){
	// 2nd last (short) slot from the end
	int offset = PAGE_SIZE - (2*sizeof(short));
	memcpy(&value, (char*)data + offset, sizeof(short));
	return 0;
}

RC incrementTotalSlots(void* data){
	short value = 0;
	getTotalSlots(data, value);
	value++;
	setTotalSlots(data, value);
	return 0;
}

//RC decrementTotalSlots(void* data){
//	short value = 0;
//	getTotalSlots(data, value);
//	value--;
//	setTotalSlots(data, value);
//	return 0;
//}

int setFreeSpace(void* data, short &value){
	// last (short) slot from the end
	int offset = PAGE_SIZE - sizeof(short);
	memcpy((char*)data + offset, &value, sizeof(short));
	return 0;
}

RC rbf_getFreeSpace(void* data, short &value){
	// last (short) slot from the end
	int offset = PAGE_SIZE - sizeof(short);
	memcpy(&value, (char*)data + offset, sizeof(short));
	return 0;
}

RC incrementFreeSpace(void* data, int &value){
	short freeSpace = 0;
	rbf_getFreeSpace(data, freeSpace);
	freeSpace += value;
	setFreeSpace(data, freeSpace);
	return 0;
}

RC decrementFreeSpace(void* data, int &value){
	short freeSpace = 0;
	rbf_getFreeSpace(data, freeSpace);
	freeSpace = freeSpace - value;
	setFreeSpace(data, freeSpace);
	return 0;
}

RC getSlotOffset(void* data, int &slotNum, short &slotOffset){

	int offset = PAGE_SIZE - (2*sizeof(short)) - ((slotNum+1)*(2*sizeof(short)));
	memcpy(&slotOffset, (char*)data + offset, sizeof(short));
	return 0;
}

RC getSlotLength(void* data, int &slotNum, short &slotLength){

	int offset = PAGE_SIZE - (2*sizeof(short)) - ((slotNum+1)*(2*sizeof(short))) + sizeof(short);
	memcpy(&slotLength, (char*)data + offset, sizeof(short));
	return 0;
}

RC setSlotOffset(void* data, int &slotNum, int &slotOffset){

	int offset = PAGE_SIZE - (2*sizeof(short)) - ((slotNum+1)*(2*sizeof(short)));
	memcpy((char*)data + offset, &slotOffset, sizeof(short));
	return 0;
}

RC setSlotLength(void* data, int &slotNum, int &slotLength){

	int offset = PAGE_SIZE - (2*sizeof(short)) - ((slotNum+1)*(2*sizeof(short))) + sizeof(short);
	memcpy((char*)data + offset, &slotLength, sizeof(short));
	return 0;
}

RC initializeNewPage(void* data){
	short freespace = PAGE_SIZE - (2*sizeof(short));
	short totalSlots = 0;
	setFreeSpace(data, freespace);
	setTotalSlots(data, totalSlots);
	return 0;
}

RC deletedSlotLocation(void* data){

	short int slotlength = 0;
	short int totalslots = 0;

	getTotalSlots(data, totalslots);

	for(int i=0; i < totalslots; i++){

		getSlotLength(data, i, slotlength);

		if(slotlength == ((short int)DEL)){
			return i;
		}
	}
	return -1;
}

RC extractNullbitArray(const void* data, int* nullbitArr, int &fields){

	int nullBytes = (int) ceil((double) fields / 8.0);

	// Extract the actual_null_bytes
	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, data, nullBytes);

	for(int i=0; i< fields; i++){
		nullbitArr[i] = byte[i/8] & (1<<(7-(i%8)));
	}

	return 0;
}

RC extractFieldData(const void* data, int &offset, AttrType type, void* fieldData, int &fieldDataSize){

	switch(type){

	case TypeInt:
		memcpy(fieldData, (char*)data + offset, sizeof(int));
		offset += sizeof(int);
		fieldDataSize = sizeof(int);
		break;

	case TypeReal:
		memcpy(fieldData, (char*)data + offset, sizeof(float));
		offset += sizeof(float);
		fieldDataSize = sizeof(float);
		break;

	case TypeVarChar:
		int varcharlength = 0;
		memcpy(&varcharlength, (char*)data + offset, sizeof(int));
		memcpy(fieldData, (char*)data + offset, sizeof(int) + varcharlength);
		offset += sizeof(int) + varcharlength;
		fieldDataSize = sizeof(int) + varcharlength;
		break;

	}
	return 0;
}

RC createRecordInMyFormat(const void *data, void* record, int &recordSize, const vector<Attribute> &recordDescriptor){

	int fields = recordDescriptor.size();
	int nullBytes = (int) ceil((double) fields / 8.0);

	int* nullbitArr = new int[fields];
	extractNullbitArray(data, nullbitArr, fields);

	// First copy only the nullBytes
	memcpy(record, data, nullBytes);

	// extract the field data from data and put into record
	int data_offset = nullBytes;
	int ptrs_offset = nullBytes;
	short record_offset = nullBytes + (fields*sizeof(short));

	int fieldDataSize;
	void* fieldData = malloc(PAGE_SIZE);

	for(int i=0; i<fields; i++){

		fieldDataSize = 0;

		if(nullbitArr[i]==0){
			extractFieldData(data, data_offset, recordDescriptor[i].type, fieldData, fieldDataSize);

			memcpy((char*)record+record_offset, fieldData, fieldDataSize);
			record_offset += fieldDataSize;

			memcpy((char*)record+ptrs_offset, &record_offset, sizeof(short));
			ptrs_offset += sizeof(short);
		}
		else{
			// Put pointer data as (-1)*(previous_record_offset) ... better way to get O(1) access
			short null_record_offset = (short)((-1)*record_offset);
			memcpy((char*)record+ptrs_offset, &null_record_offset, sizeof(short));
			ptrs_offset += sizeof(short);
		}
	}

	recordSize = record_offset;
	delete[] nullbitArr;
	return 0;
}

RC updateRightSlotDirectory(void* data, int &slotNum, int &shiftRightLength){

	short slots = 0;
	getTotalSlots(data, slots);

	short slotOffset = 0;

	// Update all the slots after that
	for(int i = slotNum+1; i<slots; i++){

		slotOffset = 0;
		getSlotOffset(data, i, slotOffset);

		int new_slotOffset = (int) (slotOffset + shiftRightLength);
		setSlotOffset(data, i, new_slotOffset);
	}

	return 0;
}

RC updateLeftSlotDirectory(void* data, int &slotNum, int &shiftLeftLength){

	short slots = 0;
	getTotalSlots(data, slots);

	short slotOffset = 0;

	// Update all the slots after that
	for(int i = slotNum+1; i<slots; i++){

		slotOffset = 0;
		getSlotOffset(data, i, slotOffset);

		int new_slotOffset = (int) (slotOffset - shiftLeftLength);
		setSlotOffset(data, i, new_slotOffset);
	}

	return 0;
}

RC freeSpaceFinder(FileHandle &fileHandle, int &recordSize, void* page, int &startPageNum, int &lastPageNum){

	short fs=0;
	for(int i = startPageNum; i<= lastPageNum; i++){
		fileHandle.readPage(i,page);
		rbf_getFreeSpace(page, fs);
		if(fs >= (recordSize + (2*sizeof(short)))){
			return i;
		}
	}
	return -1;
}

RC createHoleRecord(void* data, short &offset, short &holeLength){

	short slots = 0;
	getTotalSlots(data, slots);
	short freespace = 0;
	rbf_getFreeSpace(data, freespace);

	int freeSpaceOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short)) - freespace;
	int slotDirectoryOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short));

	void* buffer = malloc(PAGE_SIZE);

	// Copy till offset
	memcpy(buffer, data, offset);

	// Copy after skipping the hole
	memcpy((char*)buffer + offset + holeLength, (char*)data + offset + holeLength, freeSpaceOffset - (offset + holeLength));

	// Copy the slotDirectory
	memcpy((char*)buffer + slotDirectoryOffset, (char*)data + slotDirectoryOffset, (slots+1)*2*sizeof(short));

	// Write Back
	memcpy(data, buffer, PAGE_SIZE);

	free(buffer);
	return 0;
}

RC shiftLeftRecord(void* data, short &offset, short &shiftLength){

	short slots = 0;
	getTotalSlots(data, slots);
	short freespace = 0;
	rbf_getFreeSpace(data, freespace);

	int freeSpaceOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short)) - freespace;
	int slotDirectoryOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short));

	void* buffer = malloc(PAGE_SIZE);

	// Copy till offset
	memcpy(buffer, data, offset);

	// Copy after skipping the hole
	memcpy((char*)buffer + offset, (char*)data + offset + shiftLength, freeSpaceOffset - (offset + shiftLength));

	// Copy the slotDirectory
	memcpy((char*)buffer + slotDirectoryOffset, (char*)data + slotDirectoryOffset, (slots+1)*2*sizeof(short));

	// Write Back
	memcpy(data, buffer, PAGE_SIZE);

	free(buffer);
	return 0;
}

RC shiftRightRecord(void* data, short &offset, short &shiftLength){

	short slots = 0;
	getTotalSlots(data, slots);
	short freespace = 0;
	rbf_getFreeSpace(data, freespace);

	int freeSpaceOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short)) - freespace;
	int slotDirectoryOffset = PAGE_SIZE - ((slots+1)*2*sizeof(short));

	void* buffer = malloc(PAGE_SIZE);

	// Copy till offset
	memcpy(buffer, data, offset);

	// Copy after skipping the hole
	memcpy((char*)buffer + offset + shiftLength, (char*)data + offset, freeSpaceOffset - offset);

	// Copy the slotDirectory
	memcpy((char*)buffer + slotDirectoryOffset, (char*)data + slotDirectoryOffset, (slots+1)*2*sizeof(short));

	// Write Back
	memcpy(data, buffer, PAGE_SIZE);

	free(buffer);
	return 0;
}

RC insertInGivenPage(void* page, void* record, int &recordSize, RID &rid){

	short slots = 0;
	getTotalSlots(page, slots);
	short fs = 0;
	rbf_getFreeSpace(page, fs);

	int deletedPosition = deletedSlotLocation(page);
	int freespaceoffset = PAGE_SIZE - ((slots+1)*2*sizeof(short)) - fs;

	// No deleted slot to reuse
	if(deletedPosition == -1){
		memcpy((char*)page+freespaceoffset, record, recordSize);
		int slotNum = (int) slots;

		setSlotOffset(page, slotNum, freespaceoffset);
		setSlotLength(page, slotNum, recordSize);
		incrementTotalSlots(page);
		int val = (int) (recordSize + (2*sizeof(short)));
		decrementFreeSpace(page, val);
		rid.slotNum = slotNum;
	}
	else{
		// Reuse the deleted slot
		int slotNum = deletedPosition;
		short slotOffset = 0;
		getSlotOffset(page, slotNum, slotOffset);

		// Shift Right by recordSize
		short shiftLength = (short) recordSize;
		shiftRightRecord(page, slotOffset, shiftLength);
		updateRightSlotDirectory(page, slotNum, recordSize);

		memcpy((char*)page+slotOffset, record, recordSize);
		setSlotLength(page, slotNum, recordSize);

		int val = (int) (recordSize);
		decrementFreeSpace(page, val);
		rid.slotNum = slotNum;
	}

	return 0;
}

RC getDataFromMyRecordFormat(void *pageData, void* data, const RID &rid, const vector<Attribute> &recordDescriptor){

	int slotNum = (int)rid.slotNum;

	short slotOffset = 0;
	getSlotOffset(pageData, slotNum, slotOffset);
	short slotLength = 0;
	getSlotLength(pageData, slotNum, slotLength);

	void* record = malloc(PAGE_SIZE);
	memcpy(record, (char*)pageData + slotOffset, slotLength);

	int fields = recordDescriptor.size();
	int nullBytes = (int) ceil((double) fields / 8.0);

	int pointers_size = fields*sizeof(short);

	// GetData from My Data
	int offset = 0;
	memcpy((char*)data + offset, (char*)record + offset, nullBytes);
	offset += nullBytes;
	memcpy((char*)data + offset, (char*)record + offset + pointers_size, slotLength - nullBytes - pointers_size);

	free(record);
	return 0;

}

RC readForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, RID &rid_new, void *data){

	void* page_data = malloc(PAGE_SIZE);

	RID rid_temp;
	rid_temp.pageNum = rid_new.pageNum;
	rid_temp.slotNum = rid_new.slotNum;
	short slotLength = 0;
	short slotOffset = 0;
	int slotNum = 0;

	while(true){

		fileHandle.readPage(rid_temp.pageNum, page_data);
		slotNum = rid_temp.slotNum;

		getSlotLength(page_data, slotNum, slotLength);
		getSlotOffset(page_data, slotNum, slotOffset);

		if(slotLength == DEL){
			free(page_data);
			return -1;
		}
		else if(slotLength == POINT){
			memcpy(&rid_temp.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
			memcpy(&rid_temp.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));
			continue;
		}
		else{
			getDataFromMyRecordFormat(page_data, data, rid_temp, recordDescriptor);
			break;
		}

	}

	free(page_data);
	return 0;
}

RC deleteForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, RID &rid_new){

	void* page_data = malloc(PAGE_SIZE);

	RID rid_temp;
	rid_temp.pageNum = rid_new.pageNum;
	rid_temp.slotNum = rid_new.slotNum;
	short slotLength = 0;
	short slotOffset = 0;
	int slotNum = 0;
	int pageNum = 0;

	while(true){

			fileHandle.readPage(rid_temp.pageNum, page_data);

			pageNum = rid_temp.pageNum;
			slotNum = rid_temp.slotNum;
			getSlotLength(page_data, slotNum, slotLength);
			getSlotOffset(page_data, slotNum, slotOffset);

			if(slotLength == DEL){
				break;
			}
			else if(slotLength == POINT){

				// Set rid_temp for next iteration
				memcpy(&rid_temp.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
				memcpy(&rid_temp.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));

				// Delete this page record before going further
				short holeLength = sizeof(RID);
				createHoleRecord(page_data, slotOffset, holeLength);
				shiftLeftRecord(page_data, slotOffset, holeLength);
				int shiftLength = (int)holeLength;
				updateLeftSlotDirectory(page_data, slotNum, shiftLength);

				int val = (int)holeLength;
				incrementFreeSpace(page_data, val);

				int new_slotLength = DEL;
				setSlotLength(page_data, slotNum, new_slotLength);
				fileHandle.writePage(pageNum, page_data);

				continue;
			}
			else{

				// Delete the page record and break the loop
				short holeLength = slotLength;
				createHoleRecord(page_data, slotOffset, holeLength);
				shiftLeftRecord(page_data, slotOffset, holeLength);
				int shiftLength = (int)holeLength;
				updateLeftSlotDirectory(page_data, slotNum, shiftLength);

				int val = (int)slotLength;
				incrementFreeSpace(page_data, val);

				int new_slotLength = DEL;
				setSlotLength(page_data, slotNum, new_slotLength);
				fileHandle.writePage(rid_temp.pageNum, page_data);

				break;
			}


	}

	free(page_data);
	return 0;
}

RC insertForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, void* record, int &recordSize, const RID &rid, RID &rid_new){

	int totalPages = (int)(fileHandle.getNumberOfPages());
	int lastPage = totalPages-1;
	int firstPage = rid.pageNum + 1;

	void* pageData = malloc(PAGE_SIZE);

	int res = freeSpaceFinder(fileHandle, recordSize, pageData, firstPage, lastPage);

	if(res == -1){
		// Append New Page

		void* newPage = malloc(PAGE_SIZE);
		initializeNewPage(newPage);
		memcpy(newPage, record, recordSize);
		int slotNum = 0;
		int offset = 0;
		setSlotOffset(newPage, slotNum, offset);
		setSlotLength(newPage, slotNum, recordSize);
		incrementTotalSlots(newPage);
		int val = (int) (recordSize + (2*sizeof(short)));
		decrementFreeSpace(newPage, val);

		fileHandle.appendPage(newPage);
		rid_new.pageNum = totalPages;
		rid_new.slotNum = slotNum;

		free(newPage);
	}
	else{
		// FreeSpace Found in res somePage
		rid_new.pageNum = res;
		insertInGivenPage(pageData, record, recordSize, rid_new);
		fileHandle.writePage(rid_new.pageNum, pageData);
	}

	free(pageData);
	return 0;
}

RC updateForwarderRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, void *record, int &recordSize, RID &rid_new){

	void* page_data = malloc(PAGE_SIZE);

	RID rid_temp;
	rid_temp.pageNum = rid_new.pageNum;
	rid_temp.slotNum = rid_new.slotNum;
	short slotLength = 0;
	short slotOffset = 0;
	int slotNum = 0;
	int pageNum = 0;

	while(true){

			fileHandle.readPage(rid_temp.pageNum, page_data);
			pageNum = rid_temp.pageNum;
			slotNum = rid_temp.slotNum;

			getSlotLength(page_data, slotNum, slotLength);
			getSlotOffset(page_data, slotNum, slotOffset);

			if(slotLength == DEL){
				free(page_data);
				return -1;
			}
			else if(slotLength == POINT){
				// Set rid_temp for next iteration
				memcpy(&rid_temp.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
				memcpy(&rid_temp.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));
				continue;
			}
			else{
				// Update in this page [again function can be like]
				break;
			}
	}

// Normal Record - 2 cases : normal updation or creation of forwarder
	short freespace = 0;
	rbf_getFreeSpace(page_data, freespace);

	// Normal updation
	if(freespace >= (recordSize - slotLength)){
		// case-1 : record size is smaller (shiftLeft)
		// case-2 : record size is bigger (shiftRight)

		// Deleting previous record
		createHoleRecord(page_data, slotOffset, slotLength);

		if((recordSize - slotLength)>=0){
			// case-2 : record size is bigger (shiftRight)

			short shiftLength = recordSize - slotLength;
			short shiftOffset = slotOffset + slotLength;
			shiftRightRecord(page_data, shiftOffset, shiftLength);

			int newShiftLength = (int)shiftLength;
			updateRightSlotDirectory(page_data, slotNum, newShiftLength);

			memcpy((char*)page_data + slotOffset, record, recordSize);

			int new_slotLength = recordSize;
			setSlotLength(page_data, slotNum, new_slotLength);

			int val = newShiftLength;
			decrementFreeSpace(page_data, val);
		}
		else{
			// case-1 : record size is smaller (shiftLeft)

			memcpy((char*)page_data + slotOffset, record, recordSize);

			int new_slotLength = recordSize;
			setSlotLength(page_data, slotNum, new_slotLength);

			short shiftOffset = slotOffset + recordSize;
			short shiftLength = slotLength - recordSize;

			shiftLeftRecord(page_data, shiftOffset, shiftLength);
			int newShiftLength = (int)shiftLength;
			updateLeftSlotDirectory(page_data, slotNum, newShiftLength);

			int val = newShiftLength;
			incrementFreeSpace(page_data, val);
		}

		fileHandle.writePage(rid_temp.pageNum, page_data);

		free(page_data);
		return 0;
	}


	// Creation of New Forwarder
	RID rid_new_2;
	rid_new_2.pageNum = 0;
	rid_new_2.slotNum = 0;

	insertForwarderRecord(fileHandle, recordDescriptor, record, recordSize, rid_temp, rid_new_2);

	// Deleting previous record
	createHoleRecord(page_data, slotOffset, slotLength);

	memcpy((char*)page_data + slotOffset, &rid_new_2.pageNum, sizeof(unsigned));
	memcpy((char*)page_data + slotOffset + sizeof(unsigned), &rid_new_2.slotNum, sizeof(unsigned));

	int slotNum2 = rid_temp.slotNum;
	int new_slotLength = POINT;
	setSlotLength(page_data, slotNum2, new_slotLength);

	short shiftLength = slotLength - sizeof(RID);
	short shiftOffset = slotOffset + sizeof(RID);

	shiftLeftRecord(page_data, shiftOffset, shiftLength);
	int newShiftLength = (int)shiftLength;
	updateLeftSlotDirectory(page_data, slotNum2, newShiftLength);

	int val = newShiftLength;
	incrementFreeSpace(page_data, val);

	fileHandle.writePage(rid_temp.pageNum, page_data);

	free(page_data);
	return 0;

}

RC getDataInMyFormat(void *pageData, void* record, int &recordSize, const RID &rid){

	int slotNum = (int)rid.slotNum;

	short slotOffset = 0;
	getSlotOffset(pageData, slotNum, slotOffset);
	short slotLength = 0;
	getSlotLength(pageData, slotNum, slotLength);

	if(slotLength == DEL){
		recordSize = 0;
		return -1;
	}
	else if(slotLength == POINT){
		// TO-DO
	}
	else{
		recordSize = slotLength;
		memcpy(record, (char*)pageData + slotOffset, recordSize);
	}

	return 0;

}

RC readMyFormatForwarderRecord(FileHandle &fileHandle, RID &rid_new, void *record, int &recordSize){

	void* page_data = malloc(PAGE_SIZE);

	RID rid_temp;
	rid_temp.pageNum = rid_new.pageNum;
	rid_temp.slotNum = rid_new.slotNum;
	short slotLength = 0;
	short slotOffset = 0;
	int slotNum = 0;

	while(true){

		fileHandle.readPage(rid_temp.pageNum, page_data);

		slotNum = rid_temp.slotNum;
		getSlotLength(page_data, slotNum, slotLength);
		getSlotOffset(page_data, slotNum, slotOffset);

		if(slotLength == DEL){
			free(page_data);
			return -1;
		}
		else if(slotLength == POINT){
			memcpy(&rid_temp.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
			memcpy(&rid_temp.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));
			continue;
		}
		else{
			getDataInMyFormat(page_data, record, recordSize, rid_temp);
			break;
		}

	}

	free(page_data);
	return 0;
}

RC extractSingleAttributeFromMyData(void* record, int &recordSize, const vector<Attribute> &recordDescriptor, const string &attributeName, void* data, int &dataSize){

	int fields = recordDescriptor.size();
	int nullBytes = (int) ceil(fields/8.0);

	bool flag = false;
	int i=0;

	for(i=0; i<fields; i++){
		if(attributeName.compare(recordDescriptor[i].name) == 0){
			flag = true;
			break;
		}
	}

	if(flag == false){
		unsigned char* nullB = (unsigned char*)malloc(1);
		memset(nullB, 0, 1);
		unsigned char k = 128;		//1000 0000
		nullB[0] = k;
		memcpy(data, nullB , 1);
		free(nullB);
		dataSize = 1;
		return 0;
	}

	// Extract that particular field in O(1)
	short record_offset1 = 0;
	short record_offset2 = 0;
	int dataLength = 0;
	int ptrs_offset = nullBytes + (i*sizeof(short));

	memcpy(&record_offset2, (char*)record + ptrs_offset, sizeof(short));

	if(record_offset2 < 0){
		unsigned char* nullB = (unsigned char*)malloc(1);
		memset(nullB, 0, 1);
		unsigned char k = 128;		//1000 0000
		nullB[0] = k;
		memcpy(data, nullB , 1);
		free(nullB);
		dataSize = 1;
		return 0;
	}

	// Adding nullBytes in front
	unsigned char* nullB = (unsigned char*)malloc(1);
	memset(nullB, 0, 1);
	memcpy(data, nullB , 1);
	free(nullB);

	if(i==0){
		record_offset1 = nullBytes + (fields*sizeof(short));
		dataLength = record_offset2 - record_offset1;
		memcpy((char*)data + 1, (char*)record + record_offset1, dataLength);
	}
	else{
		int prev_ptrs_offset = nullBytes + ((i-1)*sizeof(short));
		memcpy(&record_offset1, (char*)record + prev_ptrs_offset, sizeof(short));

		// Getting the absolute value only
		if(record_offset1 < 0){
			record_offset1 = (-1)*record_offset1;
		}

		dataLength = record_offset2 - record_offset1;
		memcpy((char*)data + 1, (char*)record + record_offset1, dataLength);
	}

	dataSize = dataLength + 1;
	return 0;
}

RC extractConditionAttributeType(AttrType &type, const vector<Attribute> &recordDescriptor, const string &attributeName){

	for(int i=0; i<recordDescriptor.size(); i++){
		if(attributeName.compare(recordDescriptor[i].name) == 0){
			type = recordDescriptor[i].type;
			return 0;
		}
	}
	return -1;
}


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

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {

	int recordSize = 0;

	void* record = malloc(PAGE_SIZE);
	createRecordInMyFormat(data, record, recordSize, recordDescriptor);

	unsigned totalPages = (unsigned) fileHandle.getNumberOfPages();

	int insertedSlotOffset = 0;

	// First Ever Entry
	if(totalPages == 0){
		void* newPage = malloc(PAGE_SIZE);
		initializeNewPage(newPage);
		memcpy(newPage, record, recordSize);
		int slotNum = 0;
		setSlotOffset(newPage, slotNum, insertedSlotOffset);
		setSlotLength(newPage, slotNum, recordSize);
		incrementTotalSlots(newPage);
		int fs = (int) (recordSize + (2*sizeof(short)));
		decrementFreeSpace(newPage, fs);
		fileHandle.appendPage(newPage);
		rid.pageNum = 0;
		rid.slotNum = 0;
		free(newPage);
		free(record);
		fileHandle.increaseCounter(4);
		return 0;
	}

	// If Not the first entry

	// first look for freespace in last page
	unsigned lastpage = totalPages-1;
	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(lastpage, pageData);
	short fs = 0;
	rbf_getFreeSpace(pageData, fs);

	if(fs >= (recordSize + (2*sizeof(short)))){
		// insert in the last page
		rid.pageNum = lastpage;
		insertInGivenPage(pageData, record, recordSize, rid);
		fileHandle.writePage(lastpage, pageData);
		free(pageData);
		free(record);
		fileHandle.increaseCounter(4);
		return 0;
	}

	// Look for freeSpace [Both Inclusive]
	int firstPage = 0;
	int endPage = (int)(((int)lastpage)-1);
	int res = freeSpaceFinder(fileHandle, recordSize, pageData, firstPage, endPage);

	// No freespace found
	if(res == -1){
		void* newPage = malloc(PAGE_SIZE);
		initializeNewPage(newPage);
		memcpy(newPage, record, recordSize);
		int slotNum = 0;
		setSlotOffset(newPage, slotNum, insertedSlotOffset);
		setSlotLength(newPage, slotNum, recordSize);
		incrementTotalSlots(newPage);
		int val = (int) (recordSize + (2*sizeof(short)));
		decrementFreeSpace(newPage, val);
		fileHandle.appendPage(newPage);
		rid.pageNum = totalPages;
		rid.slotNum = 0;

		free(pageData);
		free(newPage);
		free(record);
		fileHandle.increaseCounter(4);
		return 0;
	}

	// FreeSpace Found in res page
	rid.pageNum = res;
	insertInGivenPage(pageData, record, recordSize, rid);
	fileHandle.writePage(res, pageData);

	free(pageData);
	free(record);
	fileHandle.increaseCounter(4);
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {

	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, pageData);

	int slotNum = (int)rid.slotNum;

	short slotOffset = 0;
	getSlotOffset(pageData, slotNum, slotOffset);

	short slotLength = 0;
	getSlotLength(pageData, slotNum, slotLength);

	if(slotLength == DEL){
		free(pageData);
		return -1;
	}
	else if(slotLength == POINT){
		RID rid_new;
		rid_new.pageNum = 0;
		rid_new.slotNum = 0;
		memcpy(&rid_new.pageNum, (char*)pageData + slotOffset, sizeof(unsigned));
		memcpy(&rid_new.slotNum, (char*)pageData + slotOffset + sizeof(unsigned), sizeof(unsigned));

		if(readForwarderRecord(fileHandle, recordDescriptor, rid_new, data) != 0){
			free(pageData);
			return -1;
		}
	}
	else{
		getDataFromMyRecordFormat(pageData, data, rid, recordDescriptor);
	}

	free(pageData);
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

	free(byte);

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
				free(str);
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
				float val1 = 0.0;
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
	delete[] bitArray;
	return 0;

}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){

	void* page_data = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, page_data);

	int slotNum = rid.slotNum;

	short slotLength = 0;
	getSlotLength(page_data, slotNum , slotLength);
	short slotOffset = 0;
	getSlotOffset(page_data, slotNum , slotOffset);

	if(slotLength == DEL){
		free(page_data);
		return -1;
	}
	else if(slotLength == POINT){

		RID rid_new;
		rid_new.pageNum = 0;
		rid_new.slotNum = 0;

		memcpy(&rid_new.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
		memcpy(&rid_new.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));

		// Delete Forwarder
		deleteForwarderRecord(fileHandle, recordDescriptor, rid_new);

		// Delete this page record after coming back
		short holeLength = sizeof(RID);
		createHoleRecord(page_data, slotOffset, holeLength);
		shiftLeftRecord(page_data, slotOffset, holeLength);
		int shiftLength = (int)holeLength;
		updateLeftSlotDirectory(page_data, slotNum, shiftLength);

		int val = (int)holeLength;
		incrementFreeSpace(page_data, val);

		int new_slotLength = DEL;
		setSlotLength(page_data, slotNum, new_slotLength);

		fileHandle.writePage(rid.pageNum, page_data);
		free(page_data);
		return 0;
	}
	else{
		createHoleRecord(page_data, slotOffset, slotLength);
		shiftLeftRecord(page_data, slotOffset, slotLength);
		int shiftLength = (int)slotLength;
		updateLeftSlotDirectory(page_data, slotNum, shiftLength);

		// No need to update slotOffset and Total slots count
		// Update only freespace, and slotLength
		int val = (int)slotLength;
		incrementFreeSpace(page_data, val);

		int new_slotLength = DEL;
		setSlotLength(page_data, slotNum, new_slotLength);

		fileHandle.writePage(rid.pageNum, page_data);
		free(page_data);
		return 0;
	}

	free(page_data);
    return -1;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){

	void* page_data = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, page_data);

	int recordSize = 0;
	void* record = malloc(PAGE_SIZE);
	createRecordInMyFormat(data, record, recordSize, recordDescriptor);

	int slotNum = rid.slotNum;

	short slotLength = 0;
	getSlotLength(page_data, slotNum , slotLength);
	short slotOffset = 0;
	getSlotOffset(page_data, slotNum , slotOffset);
	short freespace = 0;
	rbf_getFreeSpace(page_data, freespace);

	if(slotLength == DEL){
		// Record does not exist
		free(page_data);
		free(record);
		return -1;
	}
	else if(slotLength == POINT){
		RID rid_new;
		rid_new.pageNum = 0;
		rid_new.slotNum = 0;
		memcpy(&rid_new.pageNum, (char*)page_data + slotOffset, sizeof(unsigned));
		memcpy(&rid_new.slotNum, (char*)page_data + slotOffset + sizeof(unsigned), sizeof(unsigned));

		if(updateForwarderRecord(fileHandle, recordDescriptor, record, recordSize, rid_new) != 0){
			free(page_data);
			free(record);
			return -1;
		}
		free(page_data);
		free(record);
		return 0;
	}
	else{
		// Normal Record - 2 cases : normal updation or creation of forwarder

		// Normal updation
		if(freespace >= (short)(recordSize - slotLength)){
			// case-1 : record size is smaller (shiftLeft)
			// case-2 : record size is bigger (shiftRight)

			// Deleting previous record
			createHoleRecord(page_data, slotOffset, slotLength);

			if((recordSize - slotLength)>=0){
				// case-2 : record size is bigger (shiftRight)

				short shiftLength = recordSize - slotLength;
				short shiftOffset = slotOffset + slotLength;
				shiftRightRecord(page_data, shiftOffset, shiftLength);
				int newShiftLength = (int)shiftLength;
				updateRightSlotDirectory(page_data, slotNum, newShiftLength);

				memcpy((char*)page_data + slotOffset, record, recordSize);

				int new_slotLength = recordSize;
				setSlotLength(page_data, slotNum, new_slotLength);

				int val = newShiftLength;
				decrementFreeSpace(page_data, val);
			}
			else{
				// case-1 : record size is smaller (shiftLeft)

				memcpy((char*)page_data + slotOffset, record, recordSize);
				int new_slotLength = recordSize;
				setSlotLength(page_data, slotNum, new_slotLength);

				short shiftLength = slotLength - recordSize;
				short shiftOffset = slotOffset + recordSize;

				shiftLeftRecord(page_data, shiftOffset, shiftLength);
				int newShiftLength = (int)shiftLength;
				updateLeftSlotDirectory(page_data, slotNum, newShiftLength);

				int val = newShiftLength;
				incrementFreeSpace(page_data, val);
			}

			fileHandle.writePage(rid.pageNum, page_data);
			free(page_data);
			free(record);
			return 0;

		}

		// Creation of Forwarder
		RID rid_new;
		rid_new.pageNum = 0;
		rid_new.slotNum = 0;

		insertForwarderRecord(fileHandle, recordDescriptor, record, recordSize, rid, rid_new);

		// Deleting previous record
		createHoleRecord(page_data, slotOffset, slotLength);

		memcpy((char*)page_data + slotOffset, &rid_new.pageNum, sizeof(unsigned));
		memcpy((char*)page_data + slotOffset + sizeof(unsigned), &rid_new.slotNum, sizeof(unsigned));

		int new_slotLength = POINT;
		slotNum = rid.slotNum;
		setSlotLength(page_data, slotNum, new_slotLength);

		// Shift Left (Always) ---- Assumption: recordSize > sizeof(RID)
		short shiftLength = slotLength - sizeof(RID);
		short shiftOffset = slotOffset + sizeof(RID);

		shiftLeftRecord(page_data, shiftOffset, shiftLength);
		int newShiftLength = (int)shiftLength;
		updateLeftSlotDirectory(page_data, slotNum, newShiftLength);

		int val = newShiftLength;
		incrementFreeSpace(page_data, val);

		fileHandle.writePage(rid.pageNum, page_data);
		free(page_data);
		free(record);
		return 0;
	}

	free(page_data);
	free(record);
	return -1;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data){

	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, pageData);

	int slotNum = (int)rid.slotNum;

	short slotOffset = 0;
	getSlotOffset(pageData, slotNum, slotOffset);
	short slotLength = 0;
	getSlotLength(pageData, slotNum, slotLength);

	void* record = malloc(PAGE_SIZE);
	int recordSize = 0;

	if(slotLength == DEL){
		unsigned char* nullB = (unsigned char*)malloc(1);
		memset(nullB, 0, 1);
		unsigned char k = 128;		//1000 0000
		nullB[0] = k;
		memcpy(data, nullB , 1);
		free(nullB);
		free(pageData);
		free(record);
		return 0;
	}
	else if(slotLength == POINT){
		RID rid_new;
		rid_new.pageNum = 0;
		rid_new.slotNum = 0;
		memcpy(&rid_new.pageNum, (char*)pageData + slotOffset, sizeof(unsigned));
		memcpy(&rid_new.slotNum, (char*)pageData + slotOffset + sizeof(unsigned), sizeof(unsigned));

		if(readMyFormatForwarderRecord(fileHandle, rid_new, record, recordSize) != 0){
			unsigned char* nullB = (unsigned char*)malloc(1);
			memset(nullB, 0, 1);
			unsigned char k = 128;		//1000 0000
			nullB[0] = k;
			memcpy(data, nullB , 1);
			free(nullB);
			free(pageData);
			free(record);
			return 0;
		}
	}
	else{
		getDataInMyFormat(pageData, record, recordSize, rid);
	}

	int dataSize = 0;
	// Extract particular Attribute from my_record data
	extractSingleAttributeFromMyData(record, recordSize, recordDescriptor, attributeName, data, dataSize);

	free(record);
	free(pageData);
	return 0;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,
      const void *value,
      const vector<string> &attributeNames,
      RBFM_ScanIterator &rbfm_ScanIterator) {

	rbfm_ScanIterator.fileHandle = fileHandle;
	rbfm_ScanIterator.compOp = compOp;
	rbfm_ScanIterator.attributeNames = attributeNames;
	rbfm_ScanIterator.recordDescriptor = recordDescriptor;

	if(compOp != NO_OP){
		rbfm_ScanIterator.conditionAttribute = conditionAttribute;

		AttrType type;
		extractConditionAttributeType(type, recordDescriptor, conditionAttribute);
		rbfm_ScanIterator.conditionAttributeType = type;

		switch(type){

		case TypeInt: 	rbfm_ScanIterator.value = malloc(sizeof(int));
						memcpy(rbfm_ScanIterator.value, value, sizeof(int));
						break;

		case TypeReal: 	rbfm_ScanIterator.value = malloc(sizeof(float));
						memcpy(rbfm_ScanIterator.value, value, sizeof(float));
						break;

		case TypeVarChar: 	int length = 0;
							memcpy(&length, value, sizeof(int));
							rbfm_ScanIterator.value = malloc(sizeof(int) + length);
							memcpy(rbfm_ScanIterator.value, value, sizeof(int) + length);
							break;
		}


	}

	// Setting next_rid for the first iteration
	rbfm_ScanIterator.next_rid.pageNum = 0;
	rbfm_ScanIterator.next_rid.slotNum = 0;
	rbfm_ScanIterator.end_of_file = false;

	return 0;
}


// Scan_Iterator Class

RBFM_ScanIterator::RBFM_ScanIterator()
{
	end_of_file = false;
	value = NULL;
	conditionAttribute = "";
}


RBFM_ScanIterator::~RBFM_ScanIterator()
{

}


bool RBFM_ScanIterator::checkCondition(void* page_data, short &slotOffset, short &slotLength){

	void* record = malloc(PAGE_SIZE);
	int recordSize = (int) slotLength;
	memcpy(record, (char*)page_data + slotOffset, slotLength);

	void* condition_data = malloc(PAGE_SIZE);
	int data_size = 0;
	// This will return condition_data with NullBytes (remove one byte of nullbyte first)
	extractSingleAttributeFromMyData(record, recordSize, recordDescriptor, conditionAttribute, condition_data, data_size);

	free(record);

	if(data_size < 4){
		free(condition_data);
		// Condition attribute does not exist
		return false;
	}

	switch(conditionAttributeType){

	case TypeInt:{
					int conditionVal = 0;
					int val = 0;
					// Copy after removing the 1 nullbyte
					memcpy(&conditionVal, (char*)condition_data+1, sizeof(int));
					memcpy(&val, value, sizeof(int));

					free(condition_data);

					switch(compOp){
					case EQ_OP : 	if(conditionVal == val)
										return true;
									break;
					case LT_OP :		if(conditionVal < val)
										return true;
									break;
					case LE_OP :		if(conditionVal <= val)
										return true;
									break;
					case GT_OP :		if(conditionVal > val)
										return true;
									break;
					case GE_OP :		if(conditionVal >= val)
										return true;
									break;
					case NE_OP :		if(conditionVal != val)
										return true;
									break;
					case NO_OP : 	return true;
									break;
					}
				}
					break;
	case TypeReal:{
					float conditionVal = 0.0;
					float val = 0.0;
					memcpy(&conditionVal, (char*)condition_data+1, sizeof(float));
					memcpy(&val, value, sizeof(float));

					free(condition_data);

					switch(compOp){
					case EQ_OP : 	if(conditionVal == val)
										return true;
									break;
					case LT_OP :		if(conditionVal < val)
										return true;
									break;
					case LE_OP :		if(conditionVal <= val)
										return true;
									break;
					case GT_OP :		if(conditionVal > val)
										return true;
									break;
					case GE_OP :		if(conditionVal >= val)
										return true;
									break;
					case NE_OP :		if(conditionVal != val)
										return true;
									break;
					case NO_OP : 	return true;
					}
				}
					break;
	case TypeVarChar:{
						int val_len = 0;
						int condition_len = 0;

						memcpy(&condition_len, (char*)condition_data + 1, sizeof(int));
						memcpy(&val_len, value, sizeof(int));

						char* condition_val = (char*)malloc(condition_len + 1);
						char* val = (char*)malloc(val_len+1);

						memcpy(condition_val, (char*)condition_data + 1 + sizeof(int), condition_len);
						memcpy(val, (char*)value + sizeof(int), val_len);

						condition_val[condition_len] = '\0';
						val[val_len] = '\0';

						free(condition_data);

						switch(compOp){
						case EQ_OP : 	if(strcmp(condition_val, val) == 0)
											return true;
										break;
						case LT_OP :		if(strcmp(condition_val, val) < 0)
											return true;
										break;
						case LE_OP :		if(strcmp(condition_val, val) <= 0)
											return true;
										break;
						case GT_OP :		if(strcmp(condition_val, val) > 0)
											return true;
										break;
						case GE_OP :		if(strcmp(condition_val, val) >= 0)
											return true;
										break;
						case NE_OP :		if(strcmp(condition_val, val) != 0)
											return true;
										break;
						case NO_OP : 	return true;
						}
					}
						break;
	}

	return false;
}

RC RBFM_ScanIterator::findHit(RID &rid, void *data){

	if(end_of_file == true){
		// end of file reached
	    end_of_file = false;
		return -1;
	}

	// Setting up rid for this iteration
	rid.pageNum = next_rid.pageNum;
	rid.slotNum = next_rid.slotNum;

	void* page_data = malloc(PAGE_SIZE);

	bool condition_flag = false;
	int slotNum = 0;
	int pageNum = 0;
	short slotOffset = 0;
	short slotLength = 0;
	short total_slots = 0;
	int total_pages = (int) (fileHandle.getNumberOfPages());

	if(total_pages == 0){
		return -1;
	}

	while(true){

		fileHandle.readPage(rid.pageNum, page_data);
		slotNum = rid.slotNum;
		pageNum = rid.pageNum;

		getSlotOffset(page_data, slotNum, slotOffset);
		getSlotLength(page_data, slotNum, slotLength);
		getTotalSlots(page_data, total_slots);

		// Totally skip the deleted or pointed rids during the scan
		if(slotLength == DEL || slotLength == POINT){
			// Skip the RID
			slotNum++;
			if(slotNum >= total_slots){
				// Go to next page
				pageNum++;
				slotNum = 0;
				if(pageNum >= total_pages){
					// End of file reached
					free(page_data);
					return -1;
				}
			}

			rid.slotNum = slotNum;
			rid.pageNum = pageNum;
			continue;
		}
		else{
			// Check for condition
			if(compOp == NO_OP){
				condition_flag = true;
			}
			else{
				condition_flag = checkCondition(page_data, slotOffset, slotLength);
			}

			if(condition_flag == true){
				// break the infinite loop with this RID
				// Hit found
				rid.slotNum = slotNum;
				rid.pageNum = pageNum;
				break;
			}
			else{
				// skip the RID
				slotNum++;
				if(slotNum >= total_slots){
					// Go to next page
					pageNum++;
					slotNum = 0;
					if(pageNum >= total_pages){
						// End of file reached
						free(page_data);
						//cout << "Worked 3 findHit" << endl;
						return -1;
					}
				}

				rid.slotNum = slotNum;
				rid.pageNum = pageNum;
				continue;

			}

		}

	}

	// Hit Found
	// Extract your data from this hit rid

	void* record = malloc(PAGE_SIZE);
	int recordSize = 0;
	getDataInMyFormat(page_data, record, recordSize, rid);

	int hit_nullbytes = (int)ceil(attributeNames.size()/8.0);

	void* hit_data = malloc(PAGE_SIZE);

	int hit_offset = hit_nullbytes;

	int* hit_nullArr = new int[attributeNames.size()];

	void* hit_record = malloc(PAGE_SIZE);
	int hit_record_size = 0;

	for(int i=0; i<attributeNames.size(); i++){

		extractSingleAttributeFromMyData(record, recordSize, recordDescriptor, attributeNames[i], hit_record, hit_record_size);
		if(hit_record_size < 4){
			// Null data
			hit_nullArr[i] = 1;
		}
		else{
			hit_nullArr[i] = 0;
			memcpy((char*)hit_data + hit_offset, (char*)hit_record+1, hit_record_size-1);
			hit_offset += (hit_record_size-1);
		}
	}

	// Creating null bit indicators
	unsigned char* hit_nullB = (unsigned char*) malloc(hit_nullbytes);
	memset(hit_nullB, 0, hit_nullbytes);

	unsigned char k = 0;
	int a = 0;
	int b = 0;

	// NullBytes creation of output data
	for(int z=0; z< attributeNames.size(); z++){

		b = z % 8;
		if(b == 0){
			// Initialization after every 8 cycles
			k = 0;
			a = (int)(z/8);
			hit_nullB[a] = 0;
		}

		k = 1<<(7-b);

		if(hit_nullArr[z] == 1){
			hit_nullB[a] = hit_nullB[a] + k;
		}
	}

	// Write this nullBytes into hit_data
	memcpy(hit_data, hit_nullB, hit_nullbytes);

	// Writeback everything to data
	memcpy(data, hit_data, hit_offset);

	// Set rid_next for the next iteration
	int next_slotNum = rid.slotNum;
	int next_pageNum = rid.pageNum;

	next_slotNum++;
	if(next_slotNum >= total_slots){
		// Go to next page
		next_pageNum++;
		next_slotNum = 0;
		if(next_pageNum >= total_pages){
			// End of file reached
			end_of_file = true;
		}
	}

	// Set next_rid for next iteration
	next_rid.pageNum = next_pageNum;
	next_rid.slotNum = next_slotNum;

	free(record);
	free(hit_record);
	free(hit_data);
	free(page_data);
	delete[] hit_nullArr;
	return 0;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data){

	if(findHit(rid, data) == -1){
		return -1;
	}

	return 0;
}

RC RBFM_ScanIterator::close(){

	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	rbfm->closeFile(fileHandle);
//
//	if(value){
//		free(value);
//		value = NULL;
//	}

	return 0;
}





