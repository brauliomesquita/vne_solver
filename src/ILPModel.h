#ifndef ILPMODEL_H_
#define ILPMODEL_H_

#include <ilcplex/ilocplex.h>
#include <vector>
#include <fstream>

#include "Request.h"

using namespace std;

#define M 10000

typedef IloArray<IloIntVarArray> IntVarMatrix;
typedef IloArray<IntVarMatrix> IntVar3Matrix;
typedef IloArray<IntVar3Matrix> IntVar4Matrix;
typedef IloArray<IntVar4Matrix> IntVar5Matrix;

class ILPModel {
public:
	ILPModel();

	void setRelaxacao();
	double Solve(Graph *substrato, std::vector<Request*> requisicoes, bool location, bool delay, bool resilience, int fo);
	void SetCplexParameters();

private:
	bool relaxacao;
	
	IntVar4Matrix x;
	IntVar3Matrix z;
	IloIntVarArray y;
	IloCplex * problem;
};

#endif /* ILPMODEL_H_ */
