#include "Request.h"

 Request::Request(int id, int arrival, int duration, int maxD){
    this->id = id;
    this->arrival = arrival;
    this->departure = arrival + duration;
    this->duration = duration;
    this->maxD = maxD;
}

Request::~Request(){
    delete this->g;
}

 void Request::setGraph(Graph *g){
    this->g = g;
}

 int Request::getId(){
    return this->id;
}

Graph* Request::getGraph(){
    return this->g;
}

 int Request::getArrival(){
    return this->arrival;
}

int Request::getDeparture(){
    return this->departure;
}

 int Request::getDuration(){
    return this->duration;
}

 int Request::getMaxD(){
    return this->maxD;
}
