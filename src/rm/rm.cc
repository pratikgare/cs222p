
#include "rm.h"

#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "../rbf/pfm.h"

using namespace std;

RelationManager* RelationManager::instance()
{
    static RelationManager _rm;
    return &_rm;
}

RelationManager::RelationManager()
{
}

RelationManager::~RelationManager()
{
}

Attribute createAttribute(const string name, const int type, const int length){
	Attribute attr;

	attr.name = name;
	attr.type = (AttrType)type;
	attr.length = (AttrLength)length;

	return attr;
}

void createTablesAttributes(vector<Attribute> &desc){
	desc.push_back(createAttribute("table-id", TypeInt, sizeof(int)));
	desc.push_back(createAttribute("table-name", TypeVarChar, 50));
	desc.push_back(createAttribute("file-name", TypeVarChar, 50));
}

void createColumnsAttributes(vector<Attribute> &desc){
	desc.push_back(createAttribute("table-id", TypeInt, sizeof(int)));
	desc.push_back(createAttribute("column-name", TypeVarChar, 50));
	desc.push_back(createAttribute("column-type", TypeInt, sizeof(int)));
	desc.push_back(createAttribute("column-length", TypeInt, sizeof(int)));
	desc.push_back(createAttribute("column-position", TypeInt, sizeof(int)));
}

RC RelationManager::createCatalog()
{
	//Catalog Tables

	//Creating Tables table


	vector<Attribute> recordDescriptor;
	createTablesAttributes(recordDescriptor);

	//create table
	RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

	const string sysTable_tables = "Tables";
	rbfm->destroyFile(sysTable_tables);
	if(rbfm->createFile(sysTable_tables) !=0){
		return -1;
	}

	//create table
	string sysTable_columns = "Columns";
	rbfm->destroyFile(sysTable_columns);
	if(rbfm->createFile(sysTable_columns) != 0)
		return -1;

	if(createTable(sysTable_tables, recordDescriptor) != 0){
		return -1;
	}

	vector<Attribute> recordDescriptorC;
	createColumnsAttributes(recordDescriptorC);

	if(createTable("Columns", recordDescriptorC) != 0)
		return -1;

    return 0;
}

RC RelationManager::deleteCatalog()
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();

	if(rbfm->destroyFile("Tables") != 0){
		return -1;
	}

	if(rbfm->destroyFile("Columns") != 0){
		return -1;
	}

	return 0;
}

void insertTableEntry(void* buffer, const char* nullBytes, const int tid, const string tableName, const string fileName){

	int offset = 0;

	//copy null bytes
	memcpy((char*)buffer+offset, nullBytes, sizeof(char));
	offset += sizeof(char);

	//tid
	memcpy((char*)buffer+offset, &tid, sizeof(int));
	offset += sizeof(int);

	//copy tablename
	int len = tableName.length();
	memcpy((char*)buffer+offset, &len, sizeof(int));
	offset += sizeof(int);

	memcpy((char*)buffer+offset, tableName.c_str(), len);
	offset += len;

	//copy fileName
	len = fileName.length();
	memcpy((char*)buffer+offset, &len, sizeof(int));
	offset += sizeof(int);

	memcpy((char*)buffer+offset, fileName.c_str(), len);
	offset += len;
}

void insertColumnEntry(void* buffer, const char* nullBytes, const int tid, const string attr, const AttrType attrType,
						const AttrLength attrLength, const int attrPos){
	int offset = 0;

	//copy null bytes
	memcpy((char*)buffer+offset, nullBytes, sizeof(char));
	offset += sizeof(char);

	//tid
	memcpy((char*)buffer+offset, &tid, sizeof(int));
	offset += sizeof(int);

	//attr name
	int len = attr.length();
	memcpy((char*)buffer+offset, &len, sizeof(int));
	offset += sizeof(int);

	memcpy((char*)buffer+offset, attr.c_str(), len);
	offset += len;

	//attr type
	memcpy((char*)buffer+offset, &attrType, sizeof(int));
	offset += sizeof(int);

	//attr len
	memcpy((char*)buffer+offset, &attrLength, sizeof(int));
	offset += sizeof(int);

	//attrPos
	memcpy((char*)buffer+offset, &attrPos, sizeof(int));
	offset += sizeof(int);
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{

	//Insert entry in Tables
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	string sysFile = "Tables";

	if(rbfm->openFile(sysFile, fileHandle) != 0){
		return -1;
	}

	int tId = fileHandle.getCounter(4);
	tId += 1;

	RID rid;
	char* nullBytes = (char *) malloc(sizeof(char));
	memset(nullBytes, 0, sizeof(char));

	void* recordBuffer = malloc(4096);

	string fileName = tableName;

	insertTableEntry(recordBuffer, nullBytes, tId, tableName, fileName);

	vector<Attribute> recordDescriptorTables;
	createTablesAttributes(recordDescriptorTables);

	rbfm->insertRecord(fileHandle, recordDescriptorTables, recordBuffer, rid);
	void* data = malloc(4096);


	if(rbfm->closeFile(fileHandle)!=0){
		//////free(nullBytes);
		////free(recordBuffer);
		////free(data);
		return -1;
	}


	sysFile = "Columns";

	if(rbfm->openFile(sysFile, fileHandle) != 0){
		////free(nullBytes);
		////free(recordBuffer);
		////free(data);
		return -1;
	}

	vector<Attribute> recordDescriptorColumns;
	createColumnsAttributes(recordDescriptorColumns);

	for(unsigned i=0; i<attrs.size(); i++){
		RID rid;
		insertColumnEntry(recordBuffer, nullBytes, tId, attrs[i].name, attrs[i].type, attrs[i].length, i+1);
		rbfm->insertRecord(fileHandle, recordDescriptorColumns, recordBuffer, rid);

	}


	//Close the Columns file
	if(rbfm->closeFile(fileHandle)!=0){
		////free(nullBytes);
		////free(recordBuffer);
		////free(data);
		return -1;
	}



	int flag = 0;

	//create a new file for the new table
	if(tableName.compare("Tables") == 0 || tableName.compare("Columns") == 0){
		flag = 1;
	}

	if(flag == 0){
		if(rbfm->createFile(fileName) != 0){
			////free(nullBytes);
			////free(recordBuffer);
			////free(data);
			return -1;
		}

	}

	////free(nullBytes);
	////free(recordBuffer);
	////free(data);

	return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
    const string TABLES_TABLE = "Tables";
    const string COLUMNS_TABLE = "Columns";
    
	// Can't delete the system tables
    if((tableName.compare(TABLES_TABLE) == 0) || (tableName.compare(COLUMNS_TABLE) == 0)){
        return -1;
    }

       
   //delete entry from the Tables table
	//scan
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	//get table id from tables table
	
	void* value = malloc(4096);

	if(rbfm->openFile(TABLES_TABLE, fileHandle) != 0){
		////free(value);
		return -1;
	}

	//prepare data to be compared with
	int len = tableName.length();
	int offset = 0;
	memcpy((char*)value+offset, &len, sizeof(int));
	offset+=sizeof(int);
	memcpy((char*)value+offset, tableName.c_str(), len);
	offset+=len;

	RM_ScanIterator rmsi;

	vector<Attribute> sysAttr;
	createTablesAttributes(sysAttr);

	vector<string> attrNames;
	attrNames.push_back(sysAttr[0].name);

	if(rbfm->scan(fileHandle, sysAttr, sysAttr[1].name, EQ_OP, value, attrNames, rmsi.rbfmsi) != 0){
		////free(value);
		return -1;
	}

	RID rid;
	void* data = malloc(4096);

	if(rmsi.rbfmsi.getNextRecord(rid, data) !=0 ){
		////free(value);
		////free(data);
		return -1;
	}

	//get table id from the data
	int tId = 0;
	memcpy(&tId, (char*)data+sizeof(char), sizeof(int));

	//delete the entry from tables table
	if(rbfm->deleteRecord(fileHandle, sysAttr, rid) != 0){
		////free(value);
		////free(data);
		return -1;
	}

	//close the file
	if(rbfm->closeFile(fileHandle) != 0){
		////free(value);
		////free(data);
		return -1;
	}

	//cout<<tId<<endl;




	//delete entry from the Columns table
	

	if(rbfm->openFile(COLUMNS_TABLE, fileHandle) != 0){
		////free(value);
		////free(data);
		return -1;
	}

	//get columns table schema
	sysAttr.clear();
	createColumnsAttributes(sysAttr);


	//prepare scan
	attrNames.clear();
	attrNames.push_back(sysAttr[0].name);
	void* tIdBuffer = malloc(100);
	memcpy(tIdBuffer, &tId, sizeof(int));

	if(rbfm->scan(fileHandle, sysAttr, sysAttr[0].name, EQ_OP, tIdBuffer, attrNames, rmsi.rbfmsi) != 0){
		////free(value);
		////free(data);
		////free(tIdBuffer);
		return -1;
	}
	////free(tIdBuffer);



	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.rbfmsi.getNextRecord(rid, buffer) != -1){
		if(rbfm->deleteRecord(fileHandle, sysAttr, rid) != 0){
			////free(value);
			////free(data);
			////free(buffer);
			return -1;
		}

	}


	//delete the file created for that tablename
	if(rbfm->destroyFile(tableName) != 0){
		////free(value);
		////free(data);
		////free(buffer);
		return -1;
	}

	////free(value);
	////free(data);
	////free(buffer);
    return 0;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
    const string TABLES_TABLE = "Tables";
    const string COLUMNS_TABLE = "Columns";
    
    if(tableName.compare(TABLES_TABLE) == 0){
        createTablesAttributes(attrs);
        return 0;
    }
    
    if(tableName.compare(COLUMNS_TABLE) == 0){
        createColumnsAttributes(attrs);
        return 0;
    }
    
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	//get table id from tables table
	
	void* value = malloc(4096);

	if(rbfm->openFile(TABLES_TABLE, fileHandle) != 0){
		////free(value);
		return -1;
	}

	//prepare data to be compared with
	int len = tableName.length();
	int offset = 0;
	memcpy((char*)value+offset, &len, sizeof(int));
	offset+=sizeof(int);
	memcpy((char*)value+offset, tableName.c_str(), len);
	offset+=len;

	RM_ScanIterator rmsi;

	vector<Attribute> sysAttr;
	createTablesAttributes(sysAttr);

	vector<string> attrNames;
	attrNames.push_back(sysAttr[0].name);

	if(rbfm->scan(fileHandle, sysAttr, sysAttr[1].name, EQ_OP, value, attrNames, rmsi.rbfmsi) != 0){
		////free(value);
		return -1;
	}

	RID rid;
	void* data = malloc(4096);

	if(rmsi.rbfmsi.getNextRecord(rid, data) == -1 ){
		////free(value);
		////free(data);
		return -1;
	}

	//get table id from the data
	int tId = 0;
	memcpy(&tId, (char*)data+sizeof(char), sizeof(int));

	if(rbfm->closeFile(fileHandle) != 0){
		////free(value);
		////free(data);
		return -1;
	}

	//get actual attributes from columns table
	

	if(rbfm->openFile(COLUMNS_TABLE, fileHandle) != 0){
		////free(value);
		////free(data);
		return -1;
	}

	//get columns table schema
	vector<Attribute> sysAttrCol;
	createColumnsAttributes(sysAttrCol);


	//prepare scan
	vector<string> attrNamesCol;

	for(unsigned i=1; i<sysAttrCol.size()-1; i++){
		attrNamesCol.push_back(sysAttrCol[i].name);
	}


	void* tIdBuffer = malloc(100);
	memcpy(tIdBuffer, &tId, sizeof(int));

	if(rbfm->scan(fileHandle, sysAttrCol, sysAttrCol[0].name, EQ_OP, tIdBuffer, attrNamesCol, rmsi.rbfmsi) != 0){
		////free(value);
		////free(data);
		////free(tIdBuffer);
		return -1;
	}
	////free(tIdBuffer);



	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.rbfmsi.getNextRecord(rid, buffer) != -1){
		Attribute attribute;

		int offset = 1;
		//cout<<offset<<endl;
		int lenOfStr = 0;
		memcpy(&lenOfStr, (char*)buffer+offset, sizeof(int));
		//cout<<lenOfStr<<endl;

		//cout<<"before"<<offset<<endl;
		offset += sizeof(int);
		//cout<<"after"<<offset<<endl;

		char* name = (char*)malloc(lenOfStr);
		memcpy(name, (char*)buffer+offset, lenOfStr);
		offset += lenOfStr;
		//cout<<name<<endl;
		//cout<<string(name, lenOfStr)<<endl;
		attribute.name = string(name, lenOfStr);


		int type = 0;
		memcpy(&type, (char*)buffer+offset, sizeof(int));
		offset += sizeof(int);
		attribute.type = (AttrType)type;

		int length = 0;
		memcpy(&length, (char*)buffer+offset, sizeof(int));
		offset += sizeof(int);
		attribute.length = length;

		attrs.push_back(attribute);
		////free(name);
	}

	if(rbfm->closeFile(fileHandle) != 0){
		////free(value);
		////free(data);
		////free(buffer);
		return -1;
	}

	////free(value);
	////free(data);
	////free(buffer);
    return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle) != 0 ){
		return -1;
	}

	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	if(rbfm->insertRecord(fileHandle, attrs, data, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0 ){
		return -1;
	}

    return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle) != 0){
		return -1;
	}

	vector<Attribute> attributes;
	if(getAttributes(tableName, attributes) != 0){
		return -1;
	}

	if(rbfm->deleteRecord(fileHandle, attributes, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

    return 0;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
	RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle) != 0){
		return -1;
	}

	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) != 0){
		return -1;
	}

	if(rbfm->updateRecord(fileHandle, attrs, data, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

    return 0;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle)!=0){
		return -1;
	}

	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) != 0){
		return -1;
	}

	if(rbfm->readRecord(fileHandle, attrs, rid, data)!=0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

    return 0;
}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	if(rbfm->printRecord(attrs, data) != 0){
		return -1;
	}

	return 0;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle)!=0){
		return -1;
	}

	vector<Attribute> attrs;
	if(getAttributes(tableName, attrs) != 0){
		return -1;
	}

	if(rbfm->readAttribute(fileHandle, attrs, rid, attributeName, data) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	return 0;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{

	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile(tableName, fileHandle) != 0){
		return -1;
	}

	vector<Attribute> recordDescriptor;

	if(getAttributes(tableName, recordDescriptor)!=0){
		return -1;
	}

	if(rbfm->scan(fileHandle, recordDescriptor, conditionAttribute, compOp, value, attributeNames, rm_ScanIterator.rbfmsi) != 0){
		return -1;
	}

    return 0;
}

RC RM_ScanIterator::getNextTuple(RID &rid, void* data){

	if(rbfmsi.getNextRecord(rid, data) == -1){
		return -1;
	}

	return 0;
}

RC RM_ScanIterator::close(){

	if(rbfmsi.close() != 0)
		return -1;

	return 0;
}


// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
	//TODO
	//---
	//Add a chk for whether the table name is matching the systems table name
	//---
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
	//TODO
	//---
	//Add a chk for whether the table name is matching the systems table name
	//---
    return -1;
}


