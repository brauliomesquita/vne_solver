#include "Node.h"
    
Node::Node(int id, double x, double y, double cpu){
    this->id = id;
    this->x = x;
    this->y = y;
    this->cpu = cpu;
}

Node::Node(){
}

int Node::getId() const {
    return this->id;
}

double Node::getX() const {
    return this->x;
}

double Node::getY() const {
    return this->y;
}

double Node::getCPU() const {
    return this->cpu;
}
