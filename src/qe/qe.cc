
#include <qe/qe.h>
#include <rbf/pfm.h>
#include <stdlib.h>
#include <cmath>
#include <cstring>
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
						memcpy(attrVal, (char*)data+offsetDataRead+sizeof(int), length);

						string newStr((char*)attrVal, length);
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



//Projection
Project::Project(Iterator* input, const vector<string> &attrNames){
	this->project_itr = input;
	this->attrNames = attrNames;
}

RC Project::getNextTuple(void *data){

	while(this->project_itr->getNextTuple(data) != -1){

	}

	return -1;
}

void Project::getAttributes(vector<Attribute> &attrs) const{
	this->project_itr->getAttributes(attrs);
}


//Aggregate
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op){
	this->aggr_itr = input;
	this->aggrAttr = aggAttr;
	this->op = op;
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute groupAttr, AggregateOp op){
	this->aggr_itr = input;
	this->aggrAttr = aggAttr;
	this->grpAttr = groupAttr;
	this->op = op;
}

RC Aggregate::getNextTuple(void* data){

	while(this->aggr_itr->getNextTuple(data) != -1){

	}

	return -1;
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
