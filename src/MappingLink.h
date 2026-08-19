#ifndef MAPLINK_H
#define MAPLINK_H

#include "Edge.h"

class MappingLink {
	Edge phys;
	double BW;

	public:
		MappingLink(Edge l);
		void setPhysEdge(Edge p);
		Edge getPhysEdge();
		double getBW();
		void setBW(double bw);
};

#endif /* GC_H */

