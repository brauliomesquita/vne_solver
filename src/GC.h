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

struct BandwidthCoverCut {
	int physicalEdge;
	std::vector<long long> columnIds;
};

struct AcceptanceResourceCoverCut {
	int resourceKind;
	double threshold;
	std::vector<int> requests;
};

struct AcceptanceResourceProfile {
	int resourceKind;
	double threshold;
	std::vector<double> weights;
	double capacity;
};

struct AcceptanceNoGoodCut {
	std::vector<int> requests;
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
	OneDimRange constraint_bandwidth_cover;
	OneDimRange constraint_acceptance_resource_cover;
	OneDimRange constraint_acceptance_nogood;
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
		std::vector<CpuCoverCut> findViolatedCpuCoverCuts();
		std::vector<BandwidthCoverCut> findViolatedBandwidthCoverCuts();
		std::vector<AcceptanceResourceCoverCut> findViolatedAcceptanceResourceCoverCuts();
		std::vector<AcceptanceNoGoodCut> findViolatedAcceptanceNoGoodCuts();
		unsigned int separateCoverCuts();
		void buildAcceptanceResourceProfiles();
		void addCpuCoverCutToModel(const CpuCoverCut& cut);
		void addBandwidthCoverCutToModel(const BandwidthCoverCut& cut);
		void addAcceptanceResourceCoverCutToModel(const AcceptanceResourceCoverCut& cut);
		void addAcceptanceNoGoodCutToModel(const AcceptanceNoGoodCut& cut);
		bool hasCpuCoverCut(const CpuCoverCut& cut) const;
		bool hasBandwidthCoverCut(const BandwidthCoverCut& cut) const;
		bool hasAcceptanceResourceCoverCut(const AcceptanceResourceCoverCut& cut) const;
		bool hasAcceptanceNoGoodCut(const AcceptanceNoGoodCut& cut) const;
		double runPrimalHeuristic(std::vector<Column>* solution);
		double getGAP();

		void addBranchLambda(int m, int valor);
		void addBranch(Branch branch, int valor);

		double tempoMaster, tempoSub, tempoCuts, tempoHeuristica, tempoTotal, tempoRelaxacao;
		double primalHeuristicUb;
		unsigned int primalHeuristicAccepted;
		double lb, ub;
		bool sol_inteira;
		unsigned int id;
		double parentLB;
		unsigned int nCols, gCols;
		unsigned int nCuts, gCuts;
		unsigned int nCpuCuts, gCpuCuts;
		unsigned int nBandwidthCuts, gBandwidthCuts;
		unsigned int nAcceptanceCuts, gAcceptanceCuts;
		unsigned int nNoGoodCuts, gNoGoodCuts;
		unsigned int nFeasibilityChecks, nFeasibilityUnknown;
		long long nextColumnId;
		std::vector<Column> parentPool;
		std::vector<Column> forbidden;
		std::vector<Branch> branchs;
		std::vector<Column> pool;
		std::vector<CpuCoverCut> cpuCoverCuts;
		std::vector<BandwidthCoverCut> bandwidthCoverCuts;
		std::vector<AcceptanceResourceCoverCut> acceptanceResourceCoverCuts;
		std::vector<AcceptanceResourceProfile> acceptanceResourceProfiles;
		std::vector<AcceptanceNoGoodCut> acceptanceNoGoodCuts;
		std::vector<std::vector<int>> feasibleAcceptanceSets;
		std::vector<std::vector<int>> inconclusiveAcceptanceSets;

		bool location;
};

#endif /* GC_H */
