#ifndef FEASIBILITY_ORACLE_H
#define FEASIBILITY_ORACLE_H

#include <vector>

#include "Branch.h"
#include "Column.h"
#include "Request.h"

enum class FeasibilityStatus {
	Feasible,
	Infeasible,
	Unknown
};

class FeasibilityOracle {
public:
	static FeasibilityStatus Check(
		Graph* substrate,
		const std::vector<Request*>& requests,
		const std::vector<int>& selectedRequests,
		bool location,
		double timeLimitSeconds);

	static FeasibilityStatus FindEmbedding(
		Graph* substrate,
		const std::vector<Request*>& requests,
		const std::vector<int>& selectedRequests,
		bool location,
		const std::vector<Branch>& branchs,
		double timeLimitSeconds,
		std::vector<Column>* embeddingColumns,
		double* embeddingCost);
};

#endif
