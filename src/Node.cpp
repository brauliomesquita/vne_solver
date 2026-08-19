#include "Node.h"
    
Node::Node(int id, double x, double y, double cpu){
    this->id = id;
    this->x = x;
    this->y = y;
    this->cpu = cpu;
}

Node::Node(){
}

int Node::getId(){
    return this->id;
}

double Node::getX(){
    return this->x;
}

double Node::getY(){
    return this->y;
}

double Node::getCPU(){
    return this->cpu;
}
