#ifndef GRAPH_H
#define GRAPH_H

#include "Node.h"
#include "Edge.h"

#include <vector>
#include <cmath>
#include <malloc.h>

class Graph {

    int n;
    int m;
    int **adj;
    double **dist;

    std::vector<Node> nodes;
    std::vector<Edge> edges;

    public:
    Graph(int n, int m);
    ~Graph();

     void addNode(Node node);

     void addEdge(Edge edge);

     int getN();

     int getM();

     int getAdj(int i, int j);

     void setDist(Graph * sub);

     double getDist(int i, int j);

     double getCost(int m);

     std::vector<Node> getNodes();
     std::vector<Edge> getEdges();

};

#endif
