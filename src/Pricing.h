#ifndef PRICING_H
#define PRICING_H

#include <ilcplex/ilocplex.h>
#include <vector>

#include "Request.h"
#include "Column.h"
#include "Branch.h"

using namespace std;

#define M 10000

typedef IloArray<IloIntVarArray> IntVarMatrix;
typedef IloArray<IntVarMatrix> IntVar3Matrix;
typedef IloArray<IntVar3Matrix> IntVar4Matrix;
typedef IloArray<IntVar4Matrix> IntVar5Matrix;

class Pricing {
public:
	Pricing();

	void setRelaxacao();
 	void Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, IloNumArray2 gamma, IloNumArray3 alpha, IloNumArray3 pi, IloNumArray beta, std::vector<Column> * colunas, std::vector<Column> forbidden, std::vector<Branch> branchs);
	void SetCplexParameters();

private:
	
	IntVarMatrix x;
	IloIntVarArray zk;
	IloIntVarArray zl;
	IloCplex * problem;
};


#endif /* GC_H */

