#ifndef MAPEDGE_H
#define MAPEDGE_H

#include "Edge.h"
#include <vector>

class MappingEdge {
	Edge virt;
	std::vector<Edge> phys;

	public:
		MappingEdge(Edge v);
		void addPhysEdge(Edge p);
		void setVirtEdge(Edge v);
		std::vector<Edge> getPhysEdge();
		Edge getVirtEdge();
};

#endif /* GC_H */

