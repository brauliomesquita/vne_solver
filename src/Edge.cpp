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
        
int Edge::getId() const {
    return this->id;
}

 int Edge::getOrig() const {
    return this->orig;
}

 int Edge::getDest() const {
    return this->dest;
}

 double Edge::getBW() const {
    return this->bw;
}

 double Edge::getDelay() const {
    return this->delay;
}
