#include "MappingEdge.h"

MappingEdge::MappingEdge(Edge v){
	this->setVirtEdge(v);
}

void MappingEdge::addPhysEdge(Edge p){
	this->phys.push_back(p);
}

void MappingEdge::setVirtEdge(Edge v){
	this->virt = v;
}

std::vector<Edge> MappingEdge::getPhysEdge(){
	return this->phys;
}

Edge MappingEdge::getVirtEdge(){
	return this->virt;
}