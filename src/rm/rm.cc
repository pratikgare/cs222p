#include "rm.h"

#include <ix/ix.h>
#include <rbf/pfm.h>
#include <rm/rm.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>

using namespace std;

RelationManager* RelationManager::_rm = 0;

RelationManager* RelationManager::instance()
{
    if(!_rm)
        _rm = new RelationManager();

    return _rm;
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

void createIndexAttributes(vector<Attribute> &desc){
	desc.push_back(createAttribute("table-id", TypeInt, sizeof(int)));
	//desc.push_back(createAttribute("table-name", TypeVarChar, 50));
	desc.push_back(createAttribute("index-name", TypeVarChar, 50));
	desc.push_back(createAttribute("index-file-name", TypeVarChar, 50));
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

void insertIndexTableEntry(void* buffer, const char* nullBytes, const int tid, const string attrName, const string indexFileName){

	int offset = 0;

	//copy null bytes
	memcpy((char*)buffer+offset, nullBytes, sizeof(char));
	offset += sizeof(char);

	//tid
	memcpy((char*)buffer+offset, &tid, sizeof(int));
	offset += sizeof(int);

	int len = 0;
	//copy tablename
//	len = tableName.length();
//	memcpy((char*)buffer+offset, &len, sizeof(int));
//	offset += sizeof(int);
//	memcpy((char*)buffer+offset, tableName.c_str(), len);
//	offset += len;

	//attr name
	len = attrName.length();
	memcpy((char*)buffer+offset, &len, sizeof(int));
	offset += sizeof(int);
	memcpy((char*)buffer+offset, attrName.c_str(), len);
	offset += len;

	//copy indexName
	len = indexFileName.length();
	memcpy((char*)buffer+offset, &len, sizeof(int));
	offset += sizeof(int);
	memcpy((char*)buffer+offset, indexFileName.c_str(), len);
	offset += len;

}

RC RelationManager::createCatalog()
{

	RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

	//Catalog Tables
	//create Tables table
	const string sysTable_tables = "Tables";
	rbfm->destroyFile(sysTable_tables);
	if(rbfm->createFile(sysTable_tables) !=0){
		return -1;
	}

	//create columns table
	string sysTable_columns = "Columns";
	rbfm->destroyFile(sysTable_columns);
	if(rbfm->createFile(sysTable_columns) != 0){
		return -1;
	}

	vector<Attribute> recordDescriptor;
	createTablesAttributes(recordDescriptor);
	if(createTable(sysTable_tables, recordDescriptor) != 0){
		return -1;
	}

	vector<Attribute> recordDescriptorC;
	createColumnsAttributes(recordDescriptorC);
	if(createTable(sysTable_columns, recordDescriptorC) != 0){
		return -1;
	}

	//create Index table file
	string sysTable = "Index";
	rbfm->destroyFile(sysTable);
	if(rbfm->createFile(sysTable) != 0){
		return -1;
	}

	//Creating Index table
	//Preparing record descriptor for Index table
	vector<Attribute> recordDescriptorI;
	createIndexAttributes(recordDescriptorI);

	if(createTable(sysTable, recordDescriptorI) != 0){
		return -1;
	}
	//Done creating Index table

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

	if(rbfm->destroyFile("Index") != 0){
		return -1;
	}

	return 0;
}

bool isSystemTable(const string tableName){
	const string TABLES_TABLE = "Tables";
	const string COLUMNS_TABLE = "Columns";
	const string INDEX_TABLE = "Index";

	if(tableName.compare(TABLES_TABLE) == 0 || tableName.compare(COLUMNS_TABLE) == 0 || tableName.compare(INDEX_TABLE) == 0){
		return true;
	}

	return false;
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


	if(rbfm->closeFile(fileHandle)!=0){
		return -1;
	}


	sysFile = "Columns";

	if(rbfm->openFile(sysFile, fileHandle) != 0){
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
		return -1;
	}

	if(!isSystemTable(tableName) && rbfm->createFile(fileName) != 0){
		return -1;
	}

	return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
	// Can't delete the system tables
	if(isSystemTable(tableName)){
		return -1;
	}

    //delete entry from the Tables table
	//scan
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	//get table id from tables table
	
	void* value = malloc(4096);

	const string TABLES_TABLE = "Tables";

	if(rbfm->openFile(TABLES_TABLE, fileHandle) != 0){
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
		return -1;
	}

	RID rid;
	void* data = malloc(4096);

	if(rmsi.rbfmsi.getNextRecord(rid, data) !=0 ){
		return -1;
	}

	//get table id from the data
	int tId = 0;
	memcpy(&tId, (char*)data+sizeof(char), sizeof(int));

	//delete the entry from tables table
	if(rbfm->deleteRecord(fileHandle, sysAttr, rid) != 0){
		return -1;
	}

	//close the file
	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	//cout<<tId<<endl;




	//delete entry from the Columns table
	const string COLUMNS_TABLE = "Columns";
	if(rbfm->openFile(COLUMNS_TABLE, fileHandle) != 0){
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
		return -1;
	}


	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.rbfmsi.getNextRecord(rid, buffer) != -1){
		if(rbfm->deleteRecord(fileHandle, sysAttr, rid) != 0){
			return -1;
		}

	}

	//delete the file created for that tablename
	if(rbfm->destroyFile(tableName) != 0){
		return -1;
	}

    return 0;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
    const string TABLES_TABLE = "Tables";
    const string COLUMNS_TABLE = "Columns";
    const string INDEX_TABLE = "Index";

    if(tableName.compare(TABLES_TABLE) == 0){
        createTablesAttributes(attrs);
        return 0;
    }
    
    if(tableName.compare(COLUMNS_TABLE) == 0){
        createColumnsAttributes(attrs);
        return 0;
    }
    
    if(tableName.compare(INDEX_TABLE) == 0){
		createIndexAttributes(attrs);
		return 0;
	}

	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	//get table id from tables table
	
	void* value = malloc(4096);

	if(rbfm->openFile(TABLES_TABLE, fileHandle) != 0){
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
		return -1;
	}

	RID rid;
	void* data = malloc(4096);

	if(rmsi.rbfmsi.getNextRecord(rid, data) == -1 ){
		return -1;
	}

	//get table id from the data
	int tId = 0;
	memcpy(&tId, (char*)data+sizeof(char), sizeof(int));

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	//get actual attributes from columns table
	

	if(rbfm->openFile(COLUMNS_TABLE, fileHandle) != 0){
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
		return -1;
	}



	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.rbfmsi.getNextRecord(rid, buffer) != -1){
		Attribute attribute;

		int offset = 1;
		int lenOfStr = 0;
		memcpy(&lenOfStr, (char*)buffer+offset, sizeof(int));

		offset += sizeof(int);

		char* name = (char*)malloc(lenOfStr);
		memcpy(name, (char*)buffer+offset, lenOfStr);
		offset += lenOfStr;
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
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

    return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	// Can't insert into the system tables
	if(isSystemTable(tableName)){
		return -1;
	}

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
	// Can't delete from the system tables
	if(isSystemTable(tableName)){
		return -1;
	}

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
	// Can't update tuples of the system tables
	if(isSystemTable(tableName)){
		return -1;
	}

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

	if(rbfm->openFile(tableName, rm_ScanIterator.fileHandle) != 0){
		return -1;
	}

	vector<Attribute> recordDescriptor;

	if(getAttributes(tableName, recordDescriptor)!=0){
		return -1;
	}

	if(rbfm->scan(rm_ScanIterator.fileHandle, recordDescriptor, conditionAttribute, compOp, value, attributeNames, rm_ScanIterator.rbfmsi) != 0){
		return -1;
	}

    return 0;
}

RC RM_ScanIterator::getNextTuple(RID &rid, void* data){

	if(this->rbfmsi.getNextRecord(rid, data) == -1){
		return -1;
	}

	return 0;
}

RC RM_ScanIterator::close(){

	if(this->rbfmsi.close() != 0)
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

bool isAttributeOfTable(const string &tableName, const string &attributeName){

	RelationManager* rm = RelationManager::instance();

	vector<Attribute> attrs;
	rm->getAttributes(tableName, attrs);

	for(int i=0; i<attrs.size(); i++){
		if(attrs[i].name.compare(attributeName) == 0){
			return true;
		}
	}

	return false;
}

RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
	//check if the attribute is a part of the given table or not
	if(!isAttributeOfTable(attributeName, tableName)){
		cerr << "The given attribute is not an attribute of the given table" << endl;
		return -1;
	}

	string fileName = tableName+"_"+attributeName+"_"+"idx";

	IndexManager* ixManager = IndexManager::instance();
	ixManager->createFile(fileName);

	//Update the metadata file


	return 0;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
	//check if the attribute is a part of the given table or not
	if(!isAttributeOfTable(attributeName, tableName)){
		cerr << "The given attribute is not an attribute of the given table" << endl;
		return 0;
	}

	string fileName = tableName+"_"+attributeName+"_"+"idx";

	IndexManager* ixm = IndexManager::instance();
	if(ixm->destroyFile(fileName) != 0){
		return -1;
	}

	FileHandle fileHandle;
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();

	if(rbfm->openFile("Index", fileHandle) != 0){
		return -1;
	}

	vector<Attribute> rdIndexTable;
	getAttributes(tableName, rdIndexTable);

	vector<string> attrName;
	//attrName.push_back("index-file-name");
	//scan on basis of filename

	int len = fileName.length();
	void* fileNameBuffer = malloc(PAGE_SIZE);
	memcpy(fileNameBuffer, &len, sizeof(int));
	memcpy((char*)fileNameBuffer+sizeof(int), fileName.c_str(), len);

	RM_ScanIterator rmsi;

	scan(tableName, "index-file-name", EQ_OP, fileNameBuffer, attrName, rmsi);
	//rbfm->scan(fileHandle, rdIndexTable, attrName[0], EQ_OP, fileNameBuffer, attrName, rmsi.rbfmsi);

	RID rid;
	void* buffer = malloc(PAGE_SIZE);
	if(rmsi.rbfmsi.getNextRecord(rid, buffer) != 0){
		return -1;
	}

	vector<Attribute> indexAttrs;
	getAttributes("Index", indexAttrs);

	if(rbfm->deleteRecord(fileHandle, indexAttrs, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	return 0;
}

//TODO: call proj3 functions
RC RelationManager::indexScan(const string &tableName,
                      const string &attributeName,
                      const void *lowKey,
                      const void *highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      RM_IndexScanIterator &rm_IndexScanIterator)
{
	IndexManager* ixm = IndexManager::instance();
	IXFileHandle ixFileHandle;

	const string fileName = tableName+"_"+attributeName+"_"+"idx";

	if(ixm->openFile(fileName, ixFileHandle) != 0){
		return -1;
	}

	int which = -1;
	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	for(int i=0; i<attrs.size(); i++){
		if(attrs[i].name.compare(attributeName) == 0){
			which = i;
			break;
		}
	}

	if(which == -1){
		return -1;
	}

	ixm->scan(ixFileHandle, attrs[which], lowKey, highKey, lowKeyInclusive, highKeyInclusive, rm_IndexScanIterator.ixsi);

	return 0;
}

