#ifndef SOLVER_CONFIG_H
#define SOLVER_CONFIG_H

struct SolverConfig {
	double globalTimeLimitSeconds = 3600.0;
	double heuristicTimeLimitSeconds = 2.0;
	double restrictedMipTimeLimitSeconds = 2.0;
	unsigned int treeThreads = 1;
	bool rootOnly = false;
};

#endif
