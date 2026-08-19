#include "Column.h"

Column::Column(int v, int kl){
	id = -1;
	this->v = v;
	this->kl = kl;

	k = -1;
	l = -1;
	lowerBound = 0;
	upperBound = 1;
	custoFO = 0;
}

std::vector<Edge> Column::getEdges(){
	return this->edges;
}

void Column::addEdge(Edge e){
	this->edges.push_back(e);
}
