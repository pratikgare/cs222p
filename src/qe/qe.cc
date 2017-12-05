
#include <qe/qe.h>
#include <rbf/pfm.h>
#include <stdlib.h>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>


RC extractNullbitArray(const void* data, int* nullbitArr, int &fields, int &nullBytes){

	nullBytes = (int) ceil((double) fields / 8.0);

	// Extract the actual_null_bytes
	char* byte = (char*)malloc(nullBytes);
	memcpy(byte, data, nullBytes);

	for(int i=0; i< fields; i++){
		nullbitArr[i] = byte[i/8] & (1<<(7-(i%8)));
	}

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



//RC prepareProject(const void* data, const vector<Attribute> &recordDescriptor, const vector<string> &attrNames){
//
//	for(int j=0; j<attrNames.size(); j++){
//		getAttrVal(data, recordDescriptor, , attrIntVal, attrRealVal, attrVarCharVal, type);
//	}
//
//	return 0;
//}


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
//BNL
BNLJoin::BNLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages){
	this->leftIn_itr = leftIn;
	this->rightIn_tscan = rightIn;
	this->condition = condition;
	this->numPages = numPages;
}

RC BNLJoin::getNextTuple(void *data){


	return -1;
}

void BNLJoin::getAttributes(vector<Attribute> &attrs) const{

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
