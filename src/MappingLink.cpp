#include "MappingLink.h"

MappingLink::MappingLink(Edge l){
	this->setPhysEdge(l);
}

void MappingLink::setPhysEdge(Edge p){
	this->phys = p;
}

Edge MappingLink::getPhysEdge(){
	return this->phys;
}

double MappingLink::getBW(){
	return this->BW;
}

void MappingLink::setBW(double bw){
	this->BW = bw;
}