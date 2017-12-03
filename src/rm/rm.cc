#include <rm/rm.h>
#include <stdlib.h>
#include <cmath>
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

	if(!isSystemTable(tableName)){
		if(rbfm->createFile(fileName) != 0){
			return -1;
		}
	}

	return 0;
}

RC getTableId(const string &tableName, int &tId, RID &rid){

	const string TABLES_TABLE = "Tables";

	//create tablName buffer
	int len = tableName.size();
	void* tableNameBuffer = malloc(sizeof(int)+len);
	memcpy(tableNameBuffer, &len, sizeof(int));
	memcpy((char*)tableNameBuffer+sizeof(int), tableName.c_str(), len);

	RelationManager* rm = RelationManager::instance();

	vector<string> attrs;
	attrs.push_back("table-id");

	RM_ScanIterator rmsi;

	rm->scan(TABLES_TABLE, "table-name", EQ_OP, tableNameBuffer, attrs, rmsi);

	void* data = malloc(PAGE_SIZE);
	if(rmsi.getNextTuple(rid, data) == -1){
		return -1;
	}

	memcpy(&tId, (char*)data+sizeof(char), sizeof(int));

	rmsi.close();

	return 0;
}

//TODO: add the code to delete the entries from the index table
RC RelationManager::deleteTable(const string &tableName)
{
	// Can't delete the system tables
	if(isSystemTable(tableName)){
		return -1;
	}

    //delete entry from the Tables table

	//get table id from tables table
	RID rid;
	int tId = 0;
	getTableId(tableName, tId, rid);

	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;

	if(rbfm->openFile("Tables", fileHandle) != 0){
		return -1;
	}

	vector<Attribute> sysAttr;
	getAttributes("Tables", sysAttr);

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

	//get columns table schema
	sysAttr.clear();
	createColumnsAttributes(sysAttr);


	//prepare scan
	vector<string> attrNames;
	attrNames.push_back(sysAttr[0].name);
	void* tIdBuffer = malloc(100);
	memcpy(tIdBuffer, &tId, sizeof(int));

	RM_ScanIterator rmsi;

	scan(COLUMNS_TABLE, sysAttr[0].name, EQ_OP, tIdBuffer, attrNames, rmsi);

	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.getNextTuple(rid, buffer) != -1){
		if(rbfm->deleteRecord(rmsi.fileHandle, sysAttr, rid) != 0){
			return -1;
		}
	}

	//close the file
	if(rbfm->closeFile(rmsi.fileHandle) != 0){
		return -1;
	}

	//delete the file created for that tablename
	if(rbfm->destroyFile(tableName) != 0){
		return -1;
	}

	//delete the entries from index table
	const string INDEX_TABLE = "Index";




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

	//get table id from tables table
	RID rid;
	int tId = 0;
	getTableId(tableName, tId, rid);

	//get actual attributes from columns table
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

	RM_ScanIterator rmsi;

	scan(COLUMNS_TABLE, sysAttrCol[0].name, EQ_OP, tIdBuffer, attrNamesCol, rmsi);

	//reading the contents and preparing the vector of attrs needed
	void* buffer = malloc(4096);

	while(rmsi.getNextTuple(rid, buffer) != -1){
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

	if(rbfm->closeFile(rmsi.fileHandle) != 0){
		return -1;
	}

    return 0;
}

RC extractAttrs(const void* attributes, string &indexName, string &fileName){

	int offset = 1;

	int length = 0;
	//indexNameBuffer
	memcpy(&length, (char*)attributes+offset, sizeof(int));
	void* indexNameBuffer = malloc(length);
	offset += sizeof(int);
	memcpy(indexNameBuffer, (char*)attributes+offset, length);
	offset += length;
	//convert to string
	string indexNames((char*)indexNameBuffer, length);
	indexName = indexNames;
	free(indexNameBuffer);

	//fileNameBuffer
	memcpy(&length, (char*)attributes+offset, sizeof(int));
	void* fileNameBuffer = malloc(length);
	offset += sizeof(int);
	memcpy(fileNameBuffer, (char*)attributes+offset, length);
	offset += length;
	//convert to string
	string fileNames((char*)fileNameBuffer, length);
	fileName = fileNames;
	free(fileNameBuffer);

	return 0;
}

RC getKey(const void* data, const vector<Attribute> &recordDescriptor, const string &attrName, void* key, Attribute &attribute){

	int fields = recordDescriptor.size();
	int nullBytes = (int) ceil(fields/8.0);

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

	bool flag = false;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

				case TypeVarChar:{
					int length = 0;
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&length, (char*)data+offsetDataRead, sizeof(int));
						memcpy(key, (char*)data+offsetDataRead, sizeof(int)+length);
						attribute = recordDescriptor[i];
						flag = true;
					}
					offsetDataRead += sizeof(int)+length;
					break;
				}

				case TypeInt:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(key, (char*)data+offsetDataRead, sizeof(int));
						attribute = recordDescriptor[i];
						flag = true;
					}
					offsetDataRead += sizeof(int);
					break;
				}

				case TypeReal:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(key, (char*)data+offsetDataRead, sizeof(float));
						attribute = recordDescriptor[i];
						flag = true;
					}
					offsetDataRead += sizeof(float);
					break;
				}
			}
		}
		if(flag){
			break;
		}
	}
	delete[] bitArray;
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

	vector<Attribute> attrs;
	getAttributes(tableName, attrs);

	if(rbfm->openFile(tableName, fileHandle) != 0 ){
		return -1;
	}

	if(rbfm->insertRecord(fileHandle, attrs, data, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0 ){
		return -1;
	}


	// index chk
	// get table id
	int tId = 0;
	RID rid_idx;
	getTableId(tableName, tId, rid_idx);

	// get all the attributes having indexes on that table - scan the index table
	vector<string> attr;
	attr.push_back("index-name");
	attr.push_back("index-file-name");
	RM_ScanIterator rmsi;
	scan("Index", "table-id", EQ_OP, &tId, attr, rmsi);

	// we have the recordDescriptor of the table - accordingly insertEntry for respective index
	void* retData = malloc(PAGE_SIZE);
	void* key = malloc(PAGE_SIZE);
	Attribute attribute;
	IndexManager* ixm = IndexManager::instance();
	IXFileHandle ixFileHandle;

	while(rmsi.getNextTuple(rid_idx, retData) != -1){
		string attrName;
		string fileName;
		extractAttrs(retData, attrName, fileName);
		getKey(data, attrs, attrName, key, attribute);

		if(ixm->openFile(fileName, ixFileHandle) != 0){
			return -1;
		}

		if(ixm->insertEntry(ixFileHandle, attribute, key, rid) != 0){
			return -1;
		}

		if(ixm->closeFile(ixFileHandle) != 0){
			return -1;
		}
	}


    return 0;
}

//TODO: test
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

	void* data = malloc(PAGE_SIZE);

	if(rbfm->readRecord(fileHandle, attributes, rid, data) != 0){
		return -1;
	}

	if(rbfm->deleteRecord(fileHandle, attributes, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	// index chk
	// get table id
	int tId = 0;
	RID rid_idx;
	getTableId(tableName, tId, rid_idx);

	// get all the attributes having indexes on that table - scan the index table
	vector<string> attr;
	attr.push_back("index-name");
	attr.push_back("index-file-name");
	RM_ScanIterator rmsi;
	scan("Index", "table-id", EQ_OP, &tId, attr, rmsi);

	// we have the recordDescriptor of the table - accordingly insertEntry for respective index
	void* retData = malloc(PAGE_SIZE);
	void* key = malloc(PAGE_SIZE);
	Attribute attribute;
	IndexManager* ixm = IndexManager::instance();
	IXFileHandle ixFileHandle;

	while(rmsi.getNextTuple(rid_idx, retData) != -1){
		string attrName;
		string fileName;
		extractAttrs(retData, attrName, fileName);
		getKey(data, attributes, attrName, key, attribute);

		if(ixm->openFile(fileName, ixFileHandle) != 0){
			return -1;
		}

		if(ixm->deleteEntry(ixFileHandle, attribute, key, rid) != 0){
			return -1;
		}

		if(ixm->closeFile(ixFileHandle) != 0){
			return -1;
		}
	}


    return 0;
}

//TODO: test
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

	//Reading old data
	void* dataOld = malloc(PAGE_SIZE);
	if(rbfm->readRecord(fileHandle, attrs, rid, dataOld) != 0){
		return -1;
	}

	if(rbfm->updateRecord(fileHandle, attrs, data, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	// index chk
	// get table id
	int tId = 0;
	RID rid_idx;
	getTableId(tableName, tId, rid_idx);

	// get all the attributes having indexes on that table - scan the index table
	vector<string> attr;
	attr.push_back("index-name");
	attr.push_back("index-file-name");
	RM_ScanIterator rmsi;
	scan("Index", "table-id", EQ_OP, &tId, attr, rmsi);

	// we have the recordDescriptor of the table - accordingly insertEntry for respective index
	void* retData = malloc(PAGE_SIZE);
	void* key = malloc(PAGE_SIZE);
	void* keyOld = malloc(PAGE_SIZE);
	Attribute attribute;
	IndexManager* ixm = IndexManager::instance();
	IXFileHandle ixFileHandle;

	while(rmsi.getNextTuple(rid_idx, retData) != -1){
		string attrName;
		string fileName;
		extractAttrs(retData, attrName, fileName);
		getKey(data, attrs, attrName, key, attribute);
		getKey(dataOld, attrs, attrName, keyOld, attribute);

		if(ixm->openFile(fileName, ixFileHandle) != 0){
			return -1;
		}

		if(ixm->deleteEntry(ixFileHandle, attribute, keyOld, rid) != 0){
			return -1;
		}

		if(ixm->insertEntry(ixFileHandle, attribute, key, rid) != 0){
			return -1;
		}

		if(ixm->closeFile(ixFileHandle) != 0){
			return -1;
		}
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

	if(this->rbfmsi.close() != 0){
		return -1;
	}

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

bool isAttributeOfTable(const string &tableName, const string &attributeName, vector<Attribute> &attrs, Attribute &attribute){

	RelationManager* rm = RelationManager::instance();

	rm->getAttributes(tableName, attrs);

	for(unsigned i=0; i<attrs.size(); i++){
		if(attrs[i].name.compare(attributeName) == 0){
			attribute = attrs[i];
			return true;
		}
	}

	return false;
}

RC extractKey(const void* attribute, const Attribute &attr, void* key){

	int offset = 1;
	switch(attr.type){
		case TypeInt:
		case TypeReal:{
			memcpy(key, (char*)attribute+offset, sizeof(int));
			break;
		}

		case TypeVarChar:{
			int length = 0;
			memcpy(&length, (char*)attribute+offset, sizeof(int));
			memcpy(key, (char*)attribute+offset, sizeof(int)+length);
			break;
		}
	}

	return 0;
}

RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
	vector<Attribute> tableNameAttrs;
	Attribute attribute;
	//check if the attribute is a part of the given table or not
	if(!isAttributeOfTable(tableName, attributeName, tableNameAttrs, attribute)){
		cerr << "The given attribute is not an attribute of the given table" << endl;
		return -1;
	}


	//get the table id from Tables for the given tableName
	int tId = 0;
	RID rid;
	getTableId(tableName, tId, rid);

	IndexManager* ixManager = IndexManager::instance();
	//create a new index on the given attributeName ie a new file for it
	string fileName = tableName+"_"+attributeName+"_"+"idx";

	ixManager->createFile(fileName);


	//scan all the records and get the key, rid from the record
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();

	vector<string> attrNames;
	attrNames.push_back(attributeName);

	RM_ScanIterator rmsi;
	IXFileHandle ixFileHandle;
	if(ixManager->openFile(fileName, ixFileHandle) != 0){
		return -1;
	}

	scan(tableName, "", NO_OP, NULL, attrNames, rmsi);

	void* data = malloc(PAGE_SIZE);
	void* key = malloc(PAGE_SIZE);
	while(rmsi.getNextTuple(rid, data) != -1){
		extractKey(data, attribute, key);
		ixManager->insertEntry(ixFileHandle, attribute, key, rid);
	}

	//ixManager->printBtree(ixFileHandle, attribute);

	if(ixManager->closeFile(ixFileHandle) != 0){
		return -1;
	}

	free(data);
	free(key);

	//updating the metadata Index table
	//insert an entry into Index table
	const string INDEX_TABLE = "Index";

	void* indexEntry = malloc(PAGE_SIZE);
	char* nullBytes = (char*)malloc(sizeof(char));
	memset(nullBytes, 0, sizeof(char));
	insertIndexTableEntry(indexEntry, nullBytes, tId, attributeName, fileName);

	FileHandle fileHandle;
	if(rbfm->openFile(INDEX_TABLE, fileHandle) != 0){
		return -1;
	}

	vector<Attribute> rdIndexTable;
	getAttributes("Index", rdIndexTable);

	if(rbfm->insertRecord(fileHandle, rdIndexTable, indexEntry, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(fileHandle) != 0){
		return -1;
	}

	return 0;
}

//TODO: test
RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
	vector<Attribute> tableNameAttrs;
	Attribute attribute;
	//check if the attribute is a part of the given table or not
	if(!isAttributeOfTable(tableName, attributeName, tableNameAttrs, attribute)){
		cerr << "The given attribute is not an attribute of the given table" << endl;
		return -1;
	}

	string fileName = tableName+"_"+attributeName+"_"+"idx";

	//destroy file
	IndexManager* ixm = IndexManager::instance();
	if(ixm->destroyFile(fileName) != 0){
		return -1;
	}

	//Update index table
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();

	vector<Attribute> rdIndexTable;
	getAttributes(tableName, rdIndexTable);

	vector<string> attrName;

	int len = fileName.length();
	void* fileNameBuffer = malloc(PAGE_SIZE);
	memcpy(fileNameBuffer, &len, sizeof(int));
	memcpy((char*)fileNameBuffer+sizeof(int), fileName.c_str(), len);

	RM_ScanIterator rmsi;

	const string INDEX_TABLE = "Index";

	scan(INDEX_TABLE, "index-file-name", EQ_OP, fileNameBuffer, attrName, rmsi);

	RID rid;
	void* buffer = malloc(PAGE_SIZE);
	if(rmsi.getNextTuple(rid, buffer) != 0){
		return -1;
	}

	vector<Attribute> indexAttrs;
	getAttributes("Index", indexAttrs);

	if(rbfm->deleteRecord(rmsi.fileHandle, indexAttrs, rid) != 0){
		return -1;
	}

	if(rbfm->closeFile(rmsi.fileHandle) != 0){
		return -1;
	}

	return 0;
}

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

	for(unsigned i=0; i<attrs.size(); i++){
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

