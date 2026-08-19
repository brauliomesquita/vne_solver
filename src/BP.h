#ifndef BP_H
#define BP_H

#include "Request.h"
#include "Graph.h"
#include "GC.h"
#include <vector>
#include<iomanip>

class BP {

	public:
		void Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, bool useCuts, const char * outputfile);
};

#endif
