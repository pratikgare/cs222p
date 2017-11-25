#include "pfm.h"

#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <iostream>

// HiddenPageChunk Size (first 30 bytes contain alll the info)
#define HPC_CHUNK 30

using namespace std;

//Encryption key - 11694 (my D.O.B. - 01/16/1994). It can be anything depending upon the user requirement.
int encryptKey = 11694;

//My Functions
RC getEncryptValue(FILE* file);
bool createHiddenPage(FILE* file);
bool fileExists(const string &fileName);

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}

PagedFileManager::PagedFileManager()
{

}

PagedFileManager::~PagedFileManager()
{
	if(_pf_manager){
		//delete _pf_manager;
		//_pf_manager = NULL;
	}

}

RC getEncryptValue(FILE* file){

	// Get the contents of the file into a buffer
	int value = 0;

	// setting ptr to begin
	fseek(file, 0, SEEK_SET);

	// Reading hidden page data from the file.
	fread(&value, sizeof(int), 1, file);

	return value;
}

bool createHiddenPage(FILE* file){

	int offset = 0;
	void* hiddenPage = (char*) malloc(PAGE_SIZE);

	//Encrypting integer key at the beginning of the hiddenpage (initial 4 bytes of the file).
	memcpy((char*)hiddenPage, &encryptKey, sizeof(int));
	offset += sizeof(int);

	int cnt = 0;

	// Initially setting read counter value to zero. (byte 4 to byte 8 in hidden page)
	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(int));
	offset += sizeof(int);

	// Initially setting write counter value to zero. (byte 8 to byte 12 in hidden page)
	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(int));
	offset += sizeof(int);

	// Initially setting append counter value to zero. (byte 12 to byte 16 in hidden page)
	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(int));
	offset += sizeof(int);

	// Initially setting numberofrecords value to zero. (byte 16 to byte 20 in hidden page)
	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(int));
	offset += sizeof(int);

	int defaultVal = -1;

	// Initially setting pageNumOfRootNode value to zero. (byte 20 to byte 24 in hidden page)
	memcpy(((char*)hiddenPage)+offset, &defaultVal, sizeof(int));
	offset += sizeof(int);

	//Writing hidden page into a file.
	fwrite(hiddenPage, PAGE_SIZE, 1, file);
	fflush(file);

	// Free the dynamic allocated heap memory to prevent the memory leak.
	free(hiddenPage);

	return true;
}

// Method to check whether file already exists. If exists, returns true.
bool fileExists(const string &fileName) {
	FILE* file = fopen(fileName.c_str(),"rb");

	//Check if file exist.
	if(!file)
		return false;

	fclose(file);
	return true;
}


RC PagedFileManager::createFile(const string &fileName)
{
	//check if file already exists
	if(fileExists(fileName)){
		return -1;
	}

	//creating new file
	FILE* file = fopen(fileName.c_str(),"wb+");

	// If File creation fails, return error.
	if(!file){
		return -1;
	}

	// Creating Hidden Page which serves the purpose for persistence counters and file encryption.
	createHiddenPage(file);

	//close file
	fclose(file);
	return 0;
}


RC PagedFileManager::destroyFile(const string &fileName)
{
	//check if file already exists
	if(fileExists(fileName)){
		// Error in destroying file.
		if(remove(fileName.c_str()) != 0)
			return -1;

		return 0;
	}
	//if not it should fail
    return -1;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	// if file is already opened, don't let it open again.
	if(fileHandle.isOpen){
		return -1;
	}

	//check if file already exists
	if(!fileExists(fileName)){
		fileHandle.handle = NULL;
		return -1;
	}

	// Opening file in update binary mode(read and write)
	FILE* file = fopen(fileName.c_str(), "rb+");

	// Error in file opening, return error.
	if(!file)
		return -1;

	// Now, reading the encrypted key from the file to check whether this file is created by createFile method or not.
	// Getting value of the key (mode 0 - encryption key).
	int key = getEncryptValue(file);

	// If file is not created by createFile method, return error.
	if(key != encryptKey){
		fclose(file);
		return -1;
	}

	// Filehandle becomes the handle for the opened file.
	fileHandle.handle = file;
	fileHandle.isOpen = true;

	// Cache the entire Hidden Page
	fileHandle.extractHidden();

	// Extract Counters from the Hidden Page
	fileHandle.extractCountersFromHidden();

	return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	if(fileHandle.handle){

		// put counters value into hidden page chunk
		fileHandle.putCountersintoHidden();

		// persist hidden page into the disk
		fileHandle.putHiddenPageIntoDisk();

		fclose(fileHandle.handle);
		return 0;
	}
    return -1;
}


FileHandle::FileHandle()
{
	encrypt = 0;
	handle = NULL;
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    numberofrecords = 0;
    rootPageNum = -1;
    isOpen = false;
    hiddenPageChunk = NULL;
}


FileHandle::~FileHandle()
{

}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	//pgNum
	if(pageNum+1 > getNumberOfPages()){
		return -1;
	}

	int seekValue = fseek(handle, (pageNum+1)*PAGE_SIZE, SEEK_SET);

	if(seekValue!=0){
		return -1;
	}

	fread(data, PAGE_SIZE, 1, handle);

	readPageCounter++;
    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{

	if(pageNum+1>getNumberOfPages()){
		return -1;
	}
	int seekValue = fseek(handle, (pageNum+1)*PAGE_SIZE, SEEK_SET);
	if(seekValue!=0){
		return -1;
	}
	fwrite(data, PAGE_SIZE, 1, handle);
	fflush(handle);

	writePageCounter++;
	return 0;
}


RC FileHandle::appendPage(const void *data)
{
	fseek(handle, 0, SEEK_END);

	fwrite(data, PAGE_SIZE, 1, handle);
	fflush(handle);

	appendPageCounter++;

	return 0;
}


RC FileHandle::getNumberOfPages()
{
	return appendPageCounter;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	readPageCount = (unsigned) readPageCounter;
	writePageCount = (unsigned) writePageCounter;
	appendPageCount = (unsigned) appendPageCounter;
    return 0;
}


RC FileHandle::extractHidden(){

	hiddenPageChunk = malloc(HPC_CHUNK);
	fseek(handle, 0, SEEK_SET);
	fread(hiddenPageChunk, HPC_CHUNK, 1, handle);

	extractCountersFromHidden();

	return 0;
}


RC FileHandle::extractCountersFromHidden(){

	// encryptKey (0-4 bytes)
	int offset = 0;
	memcpy(&encrypt, (char*)hiddenPageChunk+offset, sizeof(unsigned));
	offset += sizeof(unsigned);

	// read (4-8 bytes)
	memcpy(&readPageCounter, (char*)hiddenPageChunk+offset, sizeof(unsigned));
	offset += sizeof(unsigned);

	// write (8-12 bytes)
	memcpy(&writePageCounter, (char*)hiddenPageChunk+offset, sizeof(unsigned));
	offset += sizeof(unsigned);

	// append (12-16 bytes)
	memcpy(&appendPageCounter, (char*)hiddenPageChunk+offset, sizeof(unsigned));
	offset += sizeof(unsigned);

	// root (20-24 bytes)
	offset += sizeof(unsigned);
	memcpy(&rootPageNum, (char*)hiddenPageChunk+offset, sizeof(unsigned));
	offset += sizeof(unsigned);

	return 0;
}


RC FileHandle::putCountersintoHidden(){

	// read (4-8 bytes)
	int offset = sizeof(int);
	memcpy((char*)hiddenPageChunk+offset, &readPageCounter, sizeof(int));
	offset += sizeof(int);

	// write (8-12 bytes)
	memcpy((char*)hiddenPageChunk+offset, &writePageCounter, sizeof(int));
	offset += sizeof(int);

	// append (12-16 bytes)
	memcpy((char*)hiddenPageChunk+offset, &appendPageCounter, sizeof(int));
	offset += sizeof(int);

	// root (20-24 bytes)
	offset += sizeof(int);
	memcpy((char*)hiddenPageChunk+offset, &rootPageNum, sizeof(int));
	offset += sizeof(int);

	return 0;
}


RC FileHandle::putHiddenPageIntoDisk(){

	fseek(handle, 0, SEEK_SET);
	fwrite(hiddenPageChunk, HPC_CHUNK, 1, handle);
	fflush(handle);

	if(hiddenPageChunk){
		free(hiddenPageChunk);
		hiddenPageChunk = NULL;
	}

	return 0;

}


RC FileHandle::getCounter(int mode)
{
	// The values of mode can only be: 0- EncryptionKey, 1- read counter, 2- write counter,
	// 3-append counter, 4-numberofrecords, 5-rootNodePage

	// Invalid value of mode, return error.
	if(mode != 0 && mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
		return -1;

	switch(mode){

	case 0: return encrypt;
				break;

	case 1: return readPageCounter;
			break;

	case 2: return writePageCounter;
				break;

	case 3: return appendPageCounter;
				break;

	case 4: return numberofrecords;
				break;

	case 5: return rootPageNum;
					break;

	}
	return 0;
}


RC FileHandle::setCounter(int mode, int value){
	// The values of mode can only be: 0- EncryptionKey, 1- read counter, 2- write counter,
	// 3-append counter, 4-numberofrecords, 5-rootNodePage

	// Invalid value of mode, return error.
	if(mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
		return -1;

	switch(mode){

	case 1: readPageCounter = value;
				break;

	case 2: writePageCounter = value;
				break;

	case 3: appendPageCounter = value;
				break;

	case 4: numberofrecords = value;
				break;

	case 5: rootPageNum = value;
				break;

	}

	return 0;
}


RC FileHandle::increaseCounter(int mode)
{
	// The values of mode can only be: 0- EncryptionKey, 1- read counter, 2- write counter,
	// 3-append counter, 4-numberofrecords, 5-rootNodePage

	// Invalid value of mode, return error.
	if(mode != 0 && mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
		return -1;

	switch(mode){

	case 1: readPageCounter++;
				break;

	case 2: writePageCounter++;
				break;

	case 3: appendPageCounter++;
				break;

	case 4: numberofrecords++;
				break;

	case 5: rootPageNum++;
				break;

	}

	return 0;

}


RC FileHandle::decreaseCounter(int mode)
{
	// The values of mode can only be: 0- EncryptionKey, 1- read counter, 2- write counter,
	// 3-append counter, 4-numberofrecords, 5-rootNodePage

	// Invalid value of mode, return error.
	if(mode != 0 && mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
		return -1;

	switch(mode){

	case 1: readPageCounter--;
				break;

	case 2: writePageCounter--;
				break;

	case 3: appendPageCounter--;
				break;

	case 4: numberofrecords--;
				break;

	case 5: rootPageNum--;
				break;

	}

	return 0;

}
