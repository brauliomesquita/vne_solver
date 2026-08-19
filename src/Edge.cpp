#include "Edge.h"

Edge::Edge (int id, int o, int d, double b, double del) {
    this->id = id;
    this->orig = o;
    this->dest = d;
    this->bw = b;
    this->delay = del;
}

Edge::Edge (){
}  
        
int Edge::getId(){
    return this->id;
}

 int Edge::getOrig(){
    return this->orig;
}

 int Edge::getDest(){
    return this->dest;
}

 double Edge::getBW(){
    return this->bw;
}

 double Edge::getDelay(){
    return this->delay;
}

