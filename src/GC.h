#ifndef GC_H
#define GC_H

#include <ilcplex/ilocplex.h>
#include <vector>
#include <utility>
#include <malloc.h>
#include <fstream>
#include <cmath>

#include "Request.h"
#include "Column.h"
#include "MappingNode.h"
#include "Pricing.h"
#include "Utility.h"
#include "Branch.h"

using namespace std;

#define M 10000

typedef IloArray<IloNumVarArray> NumVarMatrix;
typedef IloArray<NumVarMatrix> NumVar3Matrix;
typedef IloArray<NumVar3Matrix> NumVar4Matrix;
typedef IloArray<NumVar4Matrix> NumVar5Matrix;

typedef IloRangeArray OneDimRange;
typedef IloArray<IloRangeArray> TwoDimRange;
typedef IloArray<IloArray<IloRangeArray>> ThreeDimRange;
typedef IloArray<IloArray<IloArray<IloRangeArray>>> FourDimRange;

struct CpuCoverCut {
	int physicalNode;
	std::vector<std::pair<int, int>> virtualNodes;
};

class GC {
	IloEnv env;
	IloModel model;
	IloObjective objective;
	IloCplex * master;

	/* Variáveis do Modelo */
	IloNumVarArray y;
	IloNumVarArray lambda;
	NumVar3Matrix z;

	/* Restrições */
	OneDimRange constraint_bw;
	OneDimRange constraint_cpu_cover;
	TwoDimRange constraint_lambda;
	ThreeDimRange constraint_saida;
	ThreeDimRange constraint_entrada;

	Graph *substrate;
	std::vector<Request*> requests;
	
	
	public:
		GC();
		GC(GC * parent);
		void Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, bool useCuts, int *y_, Branch *branch, unsigned int *saida);
		void CreateVariables();
		void CreateObjectiveFunction();
		void CreateConstraints();
		void addColumns(std::vector<Column> colunas);
		void getDuals(IloNumArray2 * gamma, IloNumArray3 * alpha, IloNumArray3 * pi, IloNumArray * beta);
		void SetCplexParameters();
		unsigned int separateCpuCoverCuts();
		void addCpuCoverCutToModel(const CpuCoverCut& cut);
		bool hasCpuCoverCut(const CpuCoverCut& cut) const;
		double getGAP();

		void addBranchLambda(int m, int valor);
		void addBranch(Branch branch, int valor);

		double tempoMaster, tempoSub, tempoCuts, tempoTotal, tempoRelaxacao;
		double lb, ub;
		bool sol_inteira;
		unsigned int id;
		double parentLB;
		unsigned int nCols, gCols;
		unsigned int nCuts, gCuts;
		std::vector<Column> parentPool;
		std::vector<Column> forbidden;
		std::vector<Branch> branchs;
		std::vector<Column> pool;
		std::vector<CpuCoverCut> cpuCoverCuts;

		bool location;
};

#endif /* GC_H */
