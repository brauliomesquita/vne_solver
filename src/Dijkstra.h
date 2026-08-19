#ifndef DIJKSTRA_H_
#define DIJKSTRA_H_

#include "Graph.h"

#include <vector>

class Dijkstra {
public:
    std::vector<int> Run(double **bandwidth, Graph *substrate, int source,
        int destination, double requiredBandwidth);
};

#endif
