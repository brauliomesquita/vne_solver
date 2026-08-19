#ifndef COL_H
#define COL_H

#include <vector>
#include "Edge.h"

class Column {
	std::vector<Edge> edges;
	
	public:
		int v;
		int kl;
		int k, l;
		int lowerBound;
		int upperBound;
		double custoFO;

		Column(int v, int kl);

		std::vector<Edge> getEdges();
		void addEdge(Edge e);

};

#endif /* GC_H */

