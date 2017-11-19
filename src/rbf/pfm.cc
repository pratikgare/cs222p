#include "pfm.h"

#include <stdlib.h>
#include <cstdio>
#include <cstring>

using namespace std;

//My Functions
int getCounterValue(FILE* file, int mode);
RC setCounterValue(FILE* file, int, int);
bool incrementCounter(FILE* file, int mode);
bool createHiddenPage(FILE* file);
bool fileExists(const string &fileName);

//Encryption key - 11694 (my D.O.B. - 01/16/1994). It can be anything depending upon the user requirement.
int encryptKey = 11694;

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
	}
}

bool incrementCounter(FILE* file, int mode){
	// Invalid value of mode, return error. The values of mode can only be: 1-read, 2-write, 3-append, 4-increase record count
	if(mode!=1 && mode!=2 && mode!=3 && mode!=4)
		return false;

	//reading the page
	int cnt = 0;

	// Moving cursor to the beginning of the file.
	fseek(file, mode*sizeof(int), SEEK_SET);

	// Reading counter from hidden page.
	fread(&cnt, sizeof(int), 1, file);

	cnt++;

	// Retracting cursor.
	fseek(file, mode*sizeof(int), SEEK_SET);

	// Updating data in the file.
	fwrite(&cnt, sizeof(int), 1, file);
	fflush(file);

	return true;
}

bool decrementCounter(FILE* file, int mode){
	// Invalid value of mode, return error. The values of mode can only be: 1-read, 2-write, 3-append, 4-increase record count
	if(mode!=1 && mode!=2 && mode!=3 && mode!=4)
		return false;

	//reading the page
	int cnt = 0;

	// Moving cursor to the beginning of the file.
	fseek(file, mode*sizeof(int), SEEK_SET);

	// Reading counter from hidden page.
	fread(&cnt, sizeof(int), 1, file);

	cnt--;

	// Retracting cursor.
	fseek(file, mode*sizeof(int), SEEK_SET);

	// Updating data in the file.
	fwrite(&cnt, sizeof(int), 1, file);
	fflush(file);

	return true;
}

int getCounterValue(FILE* file, int mode){
	// The values of mode can only be: 0- EncryptionKey, 1- read counter, 2- write counter, 3-append counter
	// 4-numberofrecords, 5-rootNodePage

	// Invalid value of mode, return error.
	if(mode != 0 && mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
		return -1;

	// Get the contents of the file into a buffer
	int value = 0;

	// setting ptr to begin
	fseek(file, mode*sizeof(unsigned), SEEK_SET);

	// Reading hidden page data from the file.
	fread(&value, sizeof(unsigned), 1, file);

	return value;
}

RC setCounterValue(FILE* file, int mode, int val1){

	if(mode != 0 && mode != 1 && mode != 2 && mode != 3 && mode!=4 && mode!=5)
			return -1;

	// setting ptr to begin
	fseek(file, mode*sizeof(unsigned), SEEK_SET);

	// Reading hidden page data from the file.
	fwrite(&val1, sizeof(unsigned), 1, file);
	fflush(file);

	return 0;
}

bool createHiddenPage(FILE* file){
	int offset = 0;
	void* hiddenPage = (char*) malloc(PAGE_SIZE);

	//Insert 1st block of hidden page i.e encryption key
	//Encrypting integer key at the beginning of the file (initial 4 bytes of the file).
	memcpy((char*)hiddenPage, &encryptKey, sizeof(int));
	offset += sizeof(int);

	unsigned cnt = 0;

	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(unsigned));		// Initially setting read counter value to zero. (byte 4 to byte 8 in hidden page)
	offset += sizeof(unsigned);

	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(unsigned));		// Initially setting write counter value to zero. (byte 8 to byte 12 in hidden page)
	offset += sizeof(unsigned);

	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(unsigned));		// Initially setting append counter value to zero. (byte 12 to byte 16 in hidden page)
	offset += sizeof(unsigned);

	memcpy(((char*)hiddenPage)+offset, &cnt, sizeof(unsigned));		// Initially setting numberofrecords value to zero. (byte 16 to byte 20 in hidden page)
	offset += sizeof(unsigned);

	int defaultVal = -1;

	memcpy(((char*)hiddenPage)+offset, &defaultVal, sizeof(int));		// Initially setting pageNumOfRootNode value to zero. (byte 20 to byte 24 in hidden page)
	offset += sizeof(unsigned);



	//Writing hidden page into a file.
	fwrite(hiddenPage, PAGE_SIZE, 1, file);
	fflush(file);

	// Free the dynamic allocated heap memory to prevent the memory leak.
	//free(hiddenPage);

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
	//
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
	int key = getCounterValue(file, 0);

	// If file is not created by createFile method, return error.
	if(key != encryptKey){
		fclose(file);
		return -1;
	}

	// Filehandle becomes the handle for the opened file.
	fileHandle.handle = file;
	fileHandle.isOpen = true;
	return 0;
}



RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	if(fileHandle.handle){
		fileHandle.isOpen = false;
		fclose(fileHandle.handle);
		return 0;
	}
    return -1;
}


FileHandle::FileHandle()
{
	handle = NULL;
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    numberofrecords = 0;
    isOpen = false;
}


FileHandle::~FileHandle()
{

}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	int mode = 1;
	//pgNum
	if(pageNum+1 > getNumberOfPages()){
		return -1;
	}
	int seekValue = fseek(handle, (pageNum+1)*PAGE_SIZE, SEEK_SET);
	if(seekValue!=0){
		return -1;
	}
	fread(data, PAGE_SIZE, 1, handle);
	rewind(handle);
	incrementCounter(handle, mode);
    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
	int mode = 2;

	if(pageNum+1>getNumberOfPages()){
		return -1;
	}
	int seekValue = fseek(handle, (pageNum+1)*PAGE_SIZE, SEEK_SET);
	if(seekValue!=0){
		return -1;
	}
	fwrite(data, PAGE_SIZE, 1, handle);
	fflush(handle);

	rewind(handle);

	incrementCounter(handle, mode);

	return 0;
}


RC FileHandle::appendPage(const void *data)
{
	int mode = 3;

	fseek(handle, 0, SEEK_END);

	fwrite(data, PAGE_SIZE, 1, handle);
	fflush(handle);

	rewind(handle);

	incrementCounter(handle, mode);

	return 0;
}


unsigned FileHandle::getNumberOfPages()
{
	int seekValue = fseek(handle, 0, SEEK_END);
	if(seekValue!=0)
		return -1;
	long pages = ftell(handle)/PAGE_SIZE;
	rewind(handle);
	return pages-1;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	readPageCount = getCounterValue(handle, 1);
	writePageCount = getCounterValue(handle, 2);
	appendPageCount = getCounterValue(handle, 3);
	numberofrecords = getCounterValue(handle,4);

    return 0;
}

RC FileHandle::increaseCounter(int mode)
{
	incrementCounter(handle, mode);
	return 0;
}

RC FileHandle::decreaseCounter(int mode)
{
	decrementCounter(handle, mode);
	return 0;
}

RC FileHandle::setCounter(int mode, int value){
	return setCounterValue(handle, 5, value);
}

RC FileHandle::getCounter(int mode)
{
	return(getCounterValue(handle, mode));
}


