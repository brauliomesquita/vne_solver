#ifndef HEURISTICA_H_
#define HEURISTICA_H_

#include "Request.h"
#include "Dijkstra.h"
#include "Column.h"
#include <iostream>
#include <vector>
#include <time.h>  

using namespace std;

class Heuristica {
public:
	void Construtiva(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, std::vector<Column> *solucao);
};

#endif /* HEURISTICA_H_ */

