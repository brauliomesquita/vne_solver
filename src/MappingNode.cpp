#include "MappingNode.h"

MappingNode::MappingNode(Node p, Node v){
	this->setPhysNode(p);
	this->setVirtNode(v);
}

void MappingNode::setPhysNode(Node p){
	this->phys = p;
}

void MappingNode::setVirtNode(Node v){
	this->virt = v;
}

Node MappingNode::getPhysNode(){
	return this->phys;
}

Node MappingNode::getVirtNode(){
	return this->virt;
}