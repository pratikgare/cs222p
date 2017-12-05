
#include <qe/qe.h>
#include <stdlib.h>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <iostream>
#include <utility>


RC extractNullbitArray(const void* data, int* nullbitArr, const int &fields, int &nullBytes){

	nullBytes = (int) ceil((double) fields / 8.0);

	// Extract the actual_null_bytes
	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, data, nullBytes);

	for(int i=0; i< fields; i++){
		nullbitArr[i] = byte[i/8] & (1<<(7-(i%8)));
	}

	return 0;
}

RC calculateDataSize(const void* data, const vector<Attribute> &dataRd, int &dataSize){

	int fields = dataRd.size();
	int* nullbitarr = new int[fields];
	int nullBytesLength = 0;

	extractNullbitArray(data, nullbitarr, fields, nullBytesLength);

	dataSize = nullBytesLength;

	for(int i=0; i < fields; i++){

		if(nullbitarr[i] == 0){
			switch(dataRd[i].type){

				case TypeInt:		dataSize += sizeof(int);
									break;

				case TypeReal:		dataSize += sizeof(float);
									break;

				case TypeVarChar:	int len = 0;
									memcpy(&len, (char*)data + dataSize, sizeof(int));
									dataSize += sizeof(int) + len;
									break;
			}




		}

	}

	delete[] nullbitarr;
	return 0;
}

RC getAttrVal(const void* data, const vector<Attribute> &recordDescriptor, const string &attrName,
		int &attrIntVal, float &attrRealVal, string &attrVarCharVal, AttrType &attrtype){

	int fields = recordDescriptor.size();
	int nullBytes = 0;

	//GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[fields];
	extractNullbitArray(data, bitArray, fields, nullBytes);

	int offsetDataRead = 0;
	offsetDataRead+=nullBytes;

	bool flag = false;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

				case TypeVarChar:{
					int length = 0;
					memcpy(&length, (char*)data+offsetDataRead, sizeof(int));

					if(recordDescriptor[i].name.compare(attrName) == 0){
						void* attrVal = malloc(length);
						//memcpy(attrVal, (char*)data+offsetDataRead+sizeof(int), length);

						string newStr((char*)data+offsetDataRead+sizeof(int), length);
						attrVarCharVal = newStr;

						attrtype = recordDescriptor[i].type;
						flag = true;
						free(attrVal);
					}
					offsetDataRead += sizeof(int)+length;
					break;
				}

				case TypeInt:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&attrIntVal, (char*)data+offsetDataRead, sizeof(int));
						attrtype = recordDescriptor[i].type;
						flag = true;
					}
					offsetDataRead += sizeof(int);
					break;
				}

				case TypeReal:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&attrRealVal, (char*)data+offsetDataRead, sizeof(float));
						attrtype = recordDescriptor[i].type;
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

RC getAttributeType(const vector<Attribute> &recordDescriptor, const string &attrName, AttrType &attrtype){

	for(unsigned i = 0; i < recordDescriptor.size(); i++){
		if(recordDescriptor[i].name.compare(attrName) == 0){
			attrtype = recordDescriptor[i].type;
			return 0;
		}
	}

	return -1;
}

bool compareInt(const int &left, const int &right, const CompOp &op){

	switch(op){
		case EQ_OP:{
			if(left == right){
				return true;
			}
			break;
		}
		case LT_OP:{
			if(left < right){
				return true;
			}
			break;
		}
		case LE_OP:{
			if(left <= right){
				return true;
			}
			break;
		}
		case GT_OP:{
			if(left > right){
				return true;
			}
			break;
		}
		case GE_OP:{
			if(left >= right){
				return true;
			}
			break;
		}
		case NE_OP:{
			if(left != right){
				return true;
			}
			break;
		}
		case NO_OP:{
			return true;
		}
	}
	return false;
}

bool compareReal(const float &left, const float &right, const CompOp &op){

	switch(op){
		case EQ_OP:{
			if(left == right){
				return true;
			}
			break;
		}
		case LT_OP:{
			if(left < right){
				return true;
			}
			break;
		}
		case LE_OP:{
			if(left <= right){
				return true;
			}
			break;
		}
		case GT_OP:{
			if(left > right){
				return true;
			}
			break;
		}
		case GE_OP:{
			if(left >= right){
				return true;
			}
			break;
		}
		case NE_OP:{
			if(left != right){
				return true;
			}
			break;
		}
		case NO_OP:{
			return true;
		}
	}
	return false;
}

bool compareVarChar(const string &left, const string &right, const CompOp &op){

	switch(op){
		case EQ_OP:{
			if(left.compare(right) == 0){
				return true;
			}
			break;
		}
		case LT_OP:{
			if(left.compare(right) < 0){
				return true;
			}
			break;
		}
		case LE_OP:{
			if(left.compare(right) <= 0){
				return true;
			}
			break;
		}
		case GT_OP:{
			if(left.compare(right) > 0){
				return true;
			}
			break;
		}
		case GE_OP:{
			if(left.compare(right) >= 0){
				return true;
			}
			break;
		}
		case NE_OP:{
			if(left.compare(right) != 0){
				return true;
			}
			break;
		}
		case NO_OP:{
			return true;
		}
	}
	return false;
}

bool isConditionSatisfied(const void* leftData, const vector<Attribute> &leftDataRd, void* rightData,
							const vector<Attribute> &rightDataRd, const Condition &condition){

	int leftInt = 0;
	float leftReal = 0.0f;
	string leftVarChar = "";

	int rightInt = 0;
	float rightReal = 0.0f;
	string rightVarChar = "";

	AttrType leftAttrType;

	getAttrVal(leftData, leftDataRd, condition.lhsAttr, leftInt, leftReal, leftVarChar, leftAttrType);

	if(condition.bRhsIsAttr){
		//use both dataLeft and dataRight
		AttrType rightAttrType;

		getAttrVal(rightData, rightDataRd, condition.rhsAttr, rightInt, rightReal, rightVarChar, rightAttrType);

		switch(rightAttrType){
			case TypeInt:{
				return compareInt(leftInt, rightInt, condition.op);
				break;
			}
			case TypeReal:{
				return compareReal(leftReal, rightReal, condition.op);
				break;
			}
			case TypeVarChar:{
				return compareVarChar(leftVarChar, rightVarChar, condition.op);
				break;
			}
		}

	}
	else{
		//use dataLeft

		//fetch rhsValue
		switch(condition.rhsValue.type){
			case TypeInt:{
				memcpy(&rightInt, condition.rhsValue.data, sizeof(int));

				return compareInt(leftInt, rightInt, condition.op);
				break;
			}
			case TypeReal:{
				memcpy(&rightReal, condition.rhsValue.data, sizeof(float));

				return compareReal(leftReal, rightReal, condition.op);
				break;
			}
			case TypeVarChar:{
				int len = 0;
				memcpy(&len, condition.rhsValue.data, sizeof(int));
				string newStr((char*)condition.rhsValue.data+sizeof(int), len);
				rightVarChar = newStr;

				return compareVarChar(leftVarChar, rightVarChar, condition.op);
				break;
			}
		}
	}

	return false;

}


//Filter
Filter::Filter(Iterator* input, const Condition &condition) {
	this->filter_itr = input;
	this->condition = condition;
	this->getAttributes(this->filter_attrs);
}

// ... the rest of your implementations go here
RC Filter::getNextTuple(void* data){

	while(this->filter_itr->getNextTuple(data) != -1){
		vector<Attribute> dummy;
		if(isConditionSatisfied(data, this->filter_attrs, NULL, dummy, this->condition)){
			return 0;
		}
	}

	return -1;
}

void Filter::getAttributes(vector<Attribute> &attrs) const{
	this->filter_itr->getAttributes(attrs);
}


RC getAttrVal(const void* data, const vector<Attribute> &recordDescriptor, const string &attrName,
		int &attrIntVal, float &attrRealVal, string &attrVarCharVal, AttrType &attrtype, bool &isNull){

	int fields = recordDescriptor.size();
	int nullBytes = 0;

	//GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[fields];
	extractNullbitArray(data, bitArray, fields, nullBytes);

	int offsetDataRead = 0;
	offsetDataRead+=nullBytes;

	bool flag = false;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

				case TypeVarChar:{
					int length = 0;
					memcpy(&length, (char*)data+offsetDataRead, sizeof(int));

					if(recordDescriptor[i].name.compare(attrName) == 0){
						void* attrVal = malloc(length);
						//memcpy(attrVal, (char*)data+offsetDataRead+sizeof(int), length);

						string newStr((char*)data+offsetDataRead+sizeof(int), length);
						attrVarCharVal = newStr;

						attrtype = recordDescriptor[i].type;
						flag = true;
						free(attrVal);
					}
					offsetDataRead += sizeof(int)+length;
					break;
				}

				case TypeInt:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&attrIntVal, (char*)data+offsetDataRead, sizeof(int));
						attrtype = recordDescriptor[i].type;
						flag = true;
					}
					offsetDataRead += sizeof(int);
					break;
				}

				case TypeReal:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&attrRealVal, (char*)data+offsetDataRead, sizeof(float));
						attrtype = recordDescriptor[i].type;
						flag = true;
					}
					offsetDataRead += sizeof(float);
					break;
				}
			}
		}
		else{
			if(recordDescriptor[i].name.compare(attrName) == 0){
				isNull = true;
				attrtype = recordDescriptor[i].type;
			}
		}
		if(flag){
			break;
		}
	}
	delete[] bitArray;
	return 0;
}


void prepareNullBytes(unsigned char* nullByteBuffer, const int* nullByteArray, const int &size){
	unsigned char k = 0;
	int a = 0;
	int b = 0;

	// NullBytes creation of output data
	for(int z=0; z < size; z++){

		b = z % 8;
		if(b == 0){
			// Initialization after every 8 bits
			k = 0;
			a = (int)(z/8);
			nullByteBuffer[a] = 0;
		}

		k = 1 << (7-b);

		if(nullByteArray[z] == 1){
			nullByteBuffer[a] = nullByteBuffer[a] + k;
		}

	}
}

RC prepareProject(const void* data, void* retData, const vector<Attribute> &recordDescriptor, const vector<string> &attrNames){

	int *bitArray = new int[attrNames.size()];
	int nullBytes = ceil(attrNames.size()/8.0);

	for(unsigned i=0; i<attrNames.size(); i++){
		bitArray[i] = 0;
	}

	int attrIntVal = 0;
	float attrRealVal = 0;
	string attrVarCharVal = "";
	AttrType type;
	bool isNull = false;
	int write_offset = nullBytes;

	for(unsigned i=0; i<attrNames.size(); i++){
		isNull = false;
		getAttrVal(data, recordDescriptor, attrNames[i], attrIntVal, attrRealVal, attrVarCharVal, type, isNull);

		//switch - bitArray update and copy the data
		switch(type){
			case TypeInt:{
				if(isNull){
					bitArray[i] = 1;
					break;
				}
				memcpy((char*)retData+write_offset, &attrIntVal, sizeof(int));
				write_offset += sizeof(int);
				break;
			}
			case TypeReal:{
				if(isNull){
					bitArray[i] = 1;
					break;
				}
				memcpy((char*)retData+write_offset, &attrRealVal, sizeof(float));
				write_offset += sizeof(float);
				break;
			}
			case TypeVarChar:{
				if(isNull){
					bitArray[i] = 1;
					break;
				}
				int len = attrVarCharVal.length();
				memcpy((char*)retData+write_offset, &len, sizeof(int));
				write_offset += sizeof(int);
				memcpy((char*)retData+write_offset, &attrVarCharVal, len);
				write_offset += len;
				break;
			}
		}

	}

	unsigned char* nullByteBuffer = (unsigned char*)malloc(nullBytes);
	memset(nullByteBuffer, 0, nullBytes);
	prepareNullBytes(nullByteBuffer, bitArray, nullBytes);

	//memcpy
	memcpy(retData, nullByteBuffer, nullBytes);

	free(nullByteBuffer);
	delete[](bitArray);

	return 0;
}


//Projection
Project::Project(Iterator* input, const vector<string> &attrNames){
	this->project_itr = input;
	this->attrNames = attrNames;

	this->getAttributes(this->project_attrs);
}

RC Project::getNextTuple(void *data){

	int flag = 0;

	void* newData = malloc(PAGE_SIZE);
	while(this->project_itr->getNextTuple(newData) != -1){
		flag = 1;
		prepareProject(newData, data, this->project_attrs, attrNames);
		return 0;
	}
	if(flag == 0){
		return -1;
	}

	free(newData);
	return 0;
}

void Project::getAttributes(vector<Attribute> &attrs) const{
	this->project_itr->getAttributes(attrs);
}



template<class T>
void checkUpdateMin(T &min, T &val){
	if(val < min){
		min = val;
	}
}

template<class T>
void checkUpdateMax(T &max, T &val){
	if(val > max){
		max = val;
	}
}

template<class T>
void updateCount(T &cnt, T &val){
	cnt += val;
}

template<class T>
void updateSum(T &sum, T &val){
	sum += val;
}

template<class T>
void updateAvg(T &avg, T &sum, T &count){
	if(count != 0){
		avg = sum/count;
	}
	else{
		cerr<<"Count is zero. No matching elements";
	}
}

template<class T>
void calculate(const AggregateOp &op, T &dest, T &num, T &val){

	switch(op){
	//MIN=0, MAX, COUNT, SUM, AVG
		case MIN:{
			checkUpdateMin<T>(num, val);
			break;
		}
		case MAX:{
			checkUpdateMax<T>(num, val);
			break;
		}
		case COUNT:{
			updateCount<T>(num, val);
			break;
		}
		case SUM:{
			updateSum<T>(num, val);
			break;
		}
		case AVG:{
			updateAvg<float>(dest, num, val);
			break;
		}
	}
}

//Aggregate
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op){
	this->aggr_itr = input;
	this->aggrAttr = aggAttr;
	this->op = op;

	this->getAttributes(this->aggr_attrs);
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute groupAttr, AggregateOp op){
	this->aggr_itr = input;
	this->aggrAttr = aggAttr;
	this->grpAttr = groupAttr;
	this->op = op;

	this->getAttributes(this->aggr_attrs);
}

RC getAttrVal(const void* data, const vector<Attribute> &recordDescriptor, const string &attrName, float &attrVal){
	int fields = recordDescriptor.size();
	int nullBytes = 0;

	//GET BIT VECTOR FROM NULL BYTES
	int* bitArray = new int[fields];
	extractNullbitArray(data, bitArray, fields, nullBytes);

	int offsetDataRead = 0;
	offsetDataRead+=nullBytes;

	bool flag = false;
	for(int i=0; i<fields; i++){
		if(bitArray[i]==0){
			switch(recordDescriptor[i].type){

				case TypeVarChar:{
					int length = 0;
					memcpy(&length, (char*)data+offsetDataRead, sizeof(int));
					offsetDataRead += sizeof(int)+length;
					break;
				}

				case TypeInt:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						int attrIntVal = 0;
						memcpy(&attrIntVal, (char*)data+offsetDataRead, sizeof(float));
						attrVal = (float)attrIntVal;
						flag = true;
					}
					offsetDataRead += sizeof(int);
					break;
				}

				case TypeReal:{
					if(recordDescriptor[i].name.compare(attrName) == 0){
						memcpy(&attrVal, (char*)data+offsetDataRead, sizeof(float));
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

template<class T>
void prepareNewData(AggregateOp &op, void* data, T &min, T &max, T &count, T &sum, T &avg){

	int nullBytes = 1;
	void* nullByteBuffer = malloc(nullBytes);
	memset(nullByteBuffer, 0, nullBytes);

	int writeOffset = 0;
	memcpy((char*)data+writeOffset, nullByteBuffer, nullBytes);
	writeOffset += nullBytes;

	switch(op){

		case MIN:{
			memcpy((char*)data+writeOffset, &min, sizeof(T));
			break;
		}
		case MAX:{
			memcpy((char*)data+writeOffset, &max, sizeof(T));
			break;
		}
		case COUNT:{
			memcpy((char*)data+writeOffset, &count, sizeof(T));
			break;
		}
		case SUM:{
			memcpy((char*)data+writeOffset, &sum, sizeof(T));
			break;
		}
		case AVG:{
			memcpy((char*)data+writeOffset, &avg, sizeof(T));
			break;
		}

	}
}

RC Aggregate::getNextTuple(void* data){

	float count = 0;
	float avg = 0;
	float sum = 0;

	float max = FLT_MIN;
	float min = FLT_MAX;

	float dummy = 1;
	int flag = 0;

	void* newData = malloc(PAGE_SIZE);
	while(this->aggr_itr->getNextTuple(newData) != -1){
		flag = 1;
		float attrVal = 0.0f;
		getAttrVal(newData, this->aggr_attrs, this->aggrAttr.name, attrVal);

		calculate<float>(COUNT, dummy, count, dummy);
		calculate<float>(SUM, dummy, sum, attrVal);

		if(this->op == MAX){
			calculate<float>(MAX, dummy, max, attrVal);
		}
		else if(this->op == MIN){
			calculate<float>(MIN, dummy, min, attrVal);
		}
	}
	if(flag==0){
		return -1;
	}
	if(this->op == AVG){
		calculate<float>(AVG, avg, sum, count);
	}

	//put null bytes into it
	prepareNewData<float>(this->op, data, min, max, count, sum, avg);

	free(newData);
	return 0;
}

void Aggregate::getAttributes(vector<Attribute> &attrs) const{
	this->aggr_itr->getAttributes(attrs);
}

//Joins
BNLJoin::BNLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages){
	this->leftIn_itr = leftIn;
	this->rightIn_tscan = rightIn;
	this->condition = condition;
	this->numPages = numPages;
	this->leftIn_itr->getAttributes(this->leftDataRd);
	this->rightIn_tscan->getAttributes(this->rightDataRd);
	getAttributeType(this->leftDataRd, this->condition.lhsAttr, this->attrtype);
}

RC prepareRecordDescriptor(const vector<Attribute> &leftDataRd, const vector<Attribute> &rightDataRd, vector<Attribute> &returnedDataRd){

	int leftRdSize = leftDataRd.size();
	int rightRdsize = rightDataRd.size();

	for(int i=0; i < (leftRdSize + rightRdsize) ; i++){

		if(i < leftRdSize){
			// Pushing back of left recordDescriptor
			returnedDataRd.push_back(leftDataRd[i]);
		}
		else{
			// Pushing back of right recordDescriptor
			returnedDataRd.push_back(rightDataRd[i-leftRdSize]);
		}
	}
	return 0;
}

RC createNullBytesFromTwoNullBitArrays(const int* leftnullBitArr, const int &leftFields, const int* rightnullBitArr, const int &rightFields, unsigned char* returnedDataNullBytes){

	unsigned char k = 0;
	int a = 0;
	int b = 0;

	// NullBytes creation of output data
	for(int z=0; z < (leftFields + rightFields); z++){

		b = z % 8;
		if(b == 0){
			// Initialization after every 8 bits
			k = 0;
			a = (int)(z/8);
			returnedDataNullBytes[a] = 0;
		}

		k = 1 << (7-b);

		if( z < leftFields){
			if(leftnullBitArr[z] == 1){
				returnedDataNullBytes[a] = returnedDataNullBytes[a] + k;
			}
		}
		else{
			if(rightnullBitArr[z - leftFields] == 1){
				returnedDataNullBytes[a] = returnedDataNullBytes[a] + k;
			}
		}

	}

	return 0;
}

RC prepareNullBytes(const void* leftData, const int &leftFields, const void* rightData, const int &rightFields, unsigned char* returnedDataNullBytes){

	int* leftnullBitArr = new int[leftFields];
	int* rightnullBitArr = new int[rightFields];

	// extract left nullBit Array
	int leftnullBytes = 0;
	extractNullbitArray(leftData, leftnullBitArr, leftFields, leftnullBytes);

	// extract right nullBit Array
	int rightnullBytes = 0;
	extractNullbitArray(rightData, rightnullBitArr, rightFields, rightnullBytes);

	// create NullBytes from two Null Bit Arrays
	createNullBytesFromTwoNullBitArrays(leftnullBitArr, leftFields, rightnullBitArr, rightFields, returnedDataNullBytes);

	delete[] leftnullBitArr;
	delete[] rightnullBitArr;
	return 0;
}

RC joinTwoRecords(const void* leftData, const vector<Attribute> &leftDataRd, const void* rightData, const vector<Attribute> &rightDataRd,
				  void* returnedData, vector<Attribute> &returnedDataRd){



	// Prepare returned Data recordDescriptor
	prepareRecordDescriptor(leftDataRd, rightDataRd, returnedDataRd);


	// Prepare returned Data nullBytes
	int leftFields = leftDataRd.size();
	int leftDataNullBytesLength = ceil(leftFields/8.0);

	int rightFields = rightDataRd.size();
	int rightDataNullBytesLength = ceil(rightFields/8.0);

	int returnedDataFields = leftFields + rightFields;
	int returnedDataNullBytesLength = ceil(returnedDataFields/8.0);

	unsigned char* returnedDataNullBytes = (unsigned char*)malloc(returnedDataNullBytesLength);
	memset(returnedDataNullBytes, 0, returnedDataNullBytesLength);
	prepareNullBytes(leftData, leftFields, rightData, rightFields, returnedDataNullBytes);


	// Final creation of returnedData after joining

	// Appending common nullBytes
	int offset = 0;
	memcpy((char*)returnedData + offset, returnedDataNullBytes, returnedDataNullBytesLength);
	offset += returnedDataNullBytesLength;

	// Appending leftData without nullBytes
	int leftDataSize = 0;
	calculateDataSize(leftData, leftDataRd, leftDataSize);
	memcpy((char*)returnedData + offset, (char*)leftData + leftDataNullBytesLength, leftDataSize - leftDataNullBytesLength);
	offset += leftDataSize - leftDataNullBytesLength;

	// Appending rightData without nullBytes
	int rightDataSize = 0;
	calculateDataSize(rightData, rightDataRd, rightDataSize);
	memcpy((char*)returnedData + offset, (char*)rightData + rightDataNullBytesLength, rightDataSize - rightDataNullBytesLength);
	offset += rightDataSize - rightDataNullBytesLength;

	free(returnedDataNullBytes);
	return 0;
}

RC BNLJoin::createIntHashMap(int &leftInt, void* leftData){
	IntHashMap.insert(make_pair(leftInt, leftData));
	return 0;
}

RC BNLJoin::createRealHashMap(float &leftReal, void* leftData){
	RealHashMap.insert(make_pair(leftReal, leftData));
	return 0;
}

RC BNLJoin::createVarCharHashMap(string &leftVarChar, void* leftData){
	VarCharHashMap.insert(make_pair(leftVarChar, leftData));
	return 0;
}

RC BNLJoin::createHashMap(void* leftData, const vector<Attribute> &leftDataRd, const string &lhsAttr, const AttrType &attrtype){

	int leftInt = 0;
	float leftReal = 0.0;
	string leftVarChar = "";
	AttrType leftattrtype;
	getAttrVal(leftData, leftDataRd, lhsAttr, leftInt, leftReal, leftVarChar, leftattrtype);

	switch(attrtype){

	case TypeInt: 		createIntHashMap(leftInt, leftData);
						break;

	case TypeReal: 		createRealHashMap(leftReal, leftData);
						break;

	case TypeVarChar: 	createVarCharHashMap(leftVarChar, leftData);
						break;
	}

	return 0;

}

RC BNLJoin::getNextTuple(void *data){

	// Step-1 : Create In-Memory HashMap using leftIn
	void* bufferdata;
	void* prev_data;
	int leftDataSize = 0;
	int size_count = 0;
	bool end_of_file = false;
	int iteration_count = 0;

	while(!end_of_file){

		iteration_count++;

		end_of_file = true;
		size_count = 0;

		// can free this one after every while loop
		bufferdata = malloc(PAGE_SIZE);

		while(this->leftIn_itr->getNextTuple(bufferdata) != -1){

			if(iteration_count != 1){
				createHashMap(prev_data, this->leftDataRd, this->condition.lhsAttr, this->attrtype);
			}

			calculateDataSize(bufferdata, leftDataRd, leftDataSize);

			// Don't free this leftdata, otherwise you will be in trouble
			void* leftdata = malloc(leftDataSize);
			memcpy(leftdata, bufferdata, leftDataSize);

			//size_count += leftDataSize;

			if(size_count > (numPages*PAGE_SIZE)){
				// clear everything and go for the next iteration
				// Don't free this prev_data, otherwise you will be in trouble
				prev_data = malloc(leftDataSize);
				memcpy(prev_data, leftdata, leftDataSize);
				clearHashMap(this->attrtype);
				end_of_file = false;
				size_count = 0;
				break;
			}

			createHashMap(leftdata, this->leftDataRd, this->condition.lhsAttr, this->attrtype);
			size_count += leftDataSize;
		}

		// Step-2 : Iterate Through rightIn and do hash look up

		void* rightData = malloc(PAGE_SIZE);
		int IntRightData = 0;
		float RealRightData = 0.0;
		string VarcharRightData = "";
		AttrType righttype;
		void* leftData_temp;
		int leftData_temp_size = 0;
		bool hit_found = false;

		while(this->rightIn_tscan->getNextTuple(rightData) != -1){

			getAttrVal(rightData, this->rightDataRd, this->condition.rhsAttr, IntRightData, RealRightData, VarcharRightData, righttype);

			switch(this->attrtype){

			case TypeInt:	{	auto it = IntHashMap.find(IntRightData);
								if(it != IntHashMap.end()){
									// Hit Found in HashMap
									hit_found = true;
									calculateDataSize(it->second, leftDataRd, leftData_temp_size);
									leftData_temp = malloc(leftData_temp_size);
									memcpy(leftData_temp, it->second, leftData_temp_size);

								}
							}
								break;
			case TypeReal:	{	auto it = RealHashMap.find(RealRightData);
								if(it != RealHashMap.end()){
									// Hit Found in HashMap
									hit_found = true;
									calculateDataSize(it->second, leftDataRd, leftData_temp_size);
									leftData_temp = malloc(leftData_temp_size);
									memcpy(leftData_temp, it->second, leftData_temp_size);

								}
							}
								break;
			case TypeVarChar:{	auto it = VarCharHashMap.find(VarcharRightData);
								if(it != VarCharHashMap.end()){
									// Hit Found in HashMap
									hit_found = true;
									calculateDataSize(it->second, leftDataRd, leftData_temp_size);
									leftData_temp = malloc(leftData_temp_size);
									memcpy(leftData_temp, it->second, leftData_temp_size);
								}
							}
								break;
		}
			if(hit_found){
				break;
			}
		}

		if(hit_found){
			// Step-3 : Return the two joined records
			vector<Attribute> returnedDataAttributes;
			joinTwoRecords(leftData_temp, leftDataRd, rightData, rightDataRd, data, returnedDataAttributes);

			free(bufferdata);
			free(rightData);
			free(leftData_temp);
			return 0;
		}

	}

	free(bufferdata);
	return -1;
}

void BNLJoin::getAttributes(vector<Attribute> &attrs) const{
	prepareRecordDescriptor(this->leftDataRd, this->rightDataRd, attrs);
}

RC BNLJoin::clearHashMap(AttrType type){

	switch(type){
	case TypeInt:		clearIntHashMap();
						break;
	case TypeReal:		clearRealHashMap();
						break;
	case TypeVarChar:	clearVarCharHashMap();
						break;
	}

	return 0;
}

RC BNLJoin::clearIntHashMap(){

	for(auto it = IntHashMap.begin(); it != IntHashMap.end(); it++){
		free(it->second);
	}

	IntHashMap.clear();
	return 0;
}

RC BNLJoin::clearRealHashMap(){

	for(auto it = RealHashMap.begin(); it != RealHashMap.end(); it++){
		free(it->second);
	}

	RealHashMap.clear();
	return 0;
}

RC BNLJoin::clearVarCharHashMap(){

	for(auto it = VarCharHashMap.begin(); it != VarCharHashMap.end(); it++){
		free(it->second);
	}

	VarCharHashMap.clear();
	return 0;
}



//INL
INLJoin::INLJoin(Iterator* leftIn, IndexScan* rightIn, const Condition &condition){
	this->leftIn_itr = leftIn;
	this->rightIn_iscan = rightIn;
	this->condition = condition;
}

RC INLJoin::getNextTuple(void* data){

	return -1;
}

void INLJoin::getAttributes(vector<Attribute> &attrs) const{

}


//GHJ
GHJoin::GHJoin(Iterator* leftIn, Iterator* rightIn, const Condition &condition, unsigned int numPartitions){
	this->leftIn_itr = leftIn;
	this->rightIn_itr = rightIn;
	this->condition = condition;
	this->numPartitions = numPartitions;


}

RC GHJoin::getNextTuple(void* data){

	return -1;
}

void GHJoin::getAttributes(vector<Attribute> &attrs) const{

}
