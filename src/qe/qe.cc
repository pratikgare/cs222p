
#include "qe.h"


//Filter
Filter::Filter(Iterator* input, const Condition &condition) {
	this->filter_itr = input;
	this->condition = condition;
}

// ... the rest of your implementations go here
RC Filter::getNextTuple(void* data){

	while(this->filter_itr->getNextTuple(data) != -1){

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
