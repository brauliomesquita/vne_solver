#include "BP.h"
#include "Branch.h"
#include "Heuristica.h"

namespace {
constexpr char TREE_SEPARATOR = ' ';
constexpr int TREE_COLUMN_WIDTH = 15;

void writeTreeHeader(std::ostream& output) {
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "ID";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Node Obj";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Node Int";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Heur. UB";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Heur. Acc";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Best Integer";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Min LB";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "GAP (%)";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Cols";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. Cols";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Dup. Cols";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "CG Iter";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# CPU Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# BW Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Y Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# NG Cuts";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. CPU";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. BW";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. Y";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Gen. NG";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Feas. Checks";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "# Feas. Unk";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Relax Time";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "MasterTime";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "SubTime";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "CutTime";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "HeurTime";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Total Time";
	output << left << setw(28) << setfill(TREE_SEPARATOR) << "CG Stop" << endl;
}

void writeTreeRow(std::ostream& output, const GC& node, double bestUpperBound,
	double globalLowerBound) {
	const double gap = bestUpperBound > 0.0 ?
		100.0 * (1.0 - globalLowerBound / bestUpperBound) : 0.0;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.id;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.lb;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.ub;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.primalHeuristicUb;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.primalHeuristicAccepted;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << bestUpperBound;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << globalLowerBound;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << gap;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nCols;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gCols;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.duplicateColumns;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.cgIterations;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nCpuCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nBandwidthCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nAcceptanceCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nNoGoodCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gCpuCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gBandwidthCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gAcceptanceCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.gNoGoodCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nFeasibilityChecks;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.nFeasibilityUnknown;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoRelaxacao;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoMaster;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoSub;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoCuts;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoHeuristica;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.tempoTotal;
	output << left << setw(28) << setfill(TREE_SEPARATOR) << node.cgStopReason << endl;
}
}


void BP::Solve(Graph *substrate, std::vector<Request*> requests, bool location,
	bool delay, bool resilience, bool useCuts, const char * outputfile,
	const SolverConfig& config){

	std::vector<GC*> arvore;
	GC * raiz = new GC();
	Branch branch;
	int y;
	unsigned int saida;
	double bestUB = VNE_INFINITY;
	
	bool * redeAceita = new bool[requests.size()];
	for(int v=0; v<requests.size(); v++)
		redeAceita[v] = false;
	
	std::vector<Column> solucaoInicial;
	//Heuristica h;
	//h.Construtiva(substrate, requests, location, 0, 0, &solucaoInicial);
	bestUB = 0.0;
	for(int s=0; s<solucaoInicial.size(); s++){
		redeAceita[solucaoInicial[s].v] = true;
		bestUB += solucaoInicial[s].custoFO;
	}
	for(int v=0; v<requests.size(); v++)
		bestUB += 10000 * (1 -redeAceita[v]);
		
	raiz->parentPool = solucaoInicial;

	arvore.push_back(raiz);

    double worstLB;
    double tempoExecucao = 0;
	const double globalStart = get_time();
	unsigned int processedNodes = 0;
	unsigned int totalCgIterations = 0;
	unsigned int totalGeneratedColumns = 0;
	unsigned int totalDuplicateColumns = 0;
	double rootLowerBound = 0.0;
	double interruptedLowerBound = VNE_INFINITY;
	std::string terminationReason = "tree_exhausted";

    double init, end;

	std::ofstream ofs;
	ofs.open (outputfile, std::ofstream::out);// | std::ofstream::app);

	ofs << "Output for "
		<< (useCuts ? "Branch-Cut-and-Price" : "Branch-and-Price")
		<< " Algorithm" << endl;
 	ofs << "Substrate Size:       " << substrate->getN() << endl;
 	ofs << "Number os VNs:        " << requests.size() << endl;
 	ofs << "Parameters: " << endl;
 	if(location)
 		ofs << "Location" << endl;
 	if(delay)
 		ofs << "Delay" << endl;
 	if(resilience){
 		ofs << "Resilience" << endl;
 	}

 	ofs << "Objective Function:" << endl;
		ofs << "Minimize Cost" << endl;
 	ofs << endl << endl;

	ofs << "Start..." << endl;
	writeTreeHeader(ofs);
	writeTreeHeader(cout);

	while(arvore.size() != 0){
		
		tempoExecucao = get_time() - globalStart;
		if(tempoExecucao >= config.globalTimeLimitSeconds) {
			terminationReason = "global_time_limit";
			break;
		}

		double best_cost = VNE_INFINITY;
		int best_index = -1;
		for(int s=0; s<arvore.size(); s++){
			if(arvore[s]->parentLB < best_cost){
				best_cost = arvore[s]->parentLB;
				best_index = s;
			}
		}
		
		//best_index = arvore.size() - 1;	// Depth-First Search
		//best_index = 0;					// Breadth-First Search
		GC * gc = arvore[best_index];
		arvore.erase(arvore.begin() + best_index);

		if(gc->parentLB < bestUB){

			init = get_time();
			const double remainingGlobal = std::max(0.0,
				config.globalTimeLimitSeconds - (get_time() - globalStart));
			gc->Solve(substrate, requests, location, delay, resilience, useCuts,
				&y, &branch, &saida, config, remainingGlobal);
			end = get_time();
			tempoExecucao = end - globalStart;
			processedNodes++;
			totalCgIterations += gc->cgIterations;
			totalGeneratedColumns += gc->gCols;
			totalDuplicateColumns += gc->duplicateColumns;
			if (gc->id == 1) rootLowerBound = gc->lb;

			if(gc->ub < bestUB){
				bestUB = gc->ub;
			}

			if(gc->relaxationComplete && !config.rootOnly && saida == 1 &&
				gc->lb < bestUB){
				for(int i=0; i<=1; i++){
					GC * filho = new GC(gc);
					filho->addBranch(branch, i);
					filho->id = gc->id * 2 + i;
					arvore.push_back(filho);
				}
				
			}

			worstLB = bestUB;
			for(int s=0; s<arvore.size(); s++){
				if(arvore[s]->parentLB < worstLB){
					worstLB = arvore[s]->parentLB;
				}
			}

			writeTreeRow(ofs, *gc, bestUB, worstLB);
			writeTreeRow(cout, *gc, bestUB, worstLB);

			if (!gc->relaxationComplete) {
				terminationReason = gc->cgStopReason;
				interruptedLowerBound = gc->lb;
				delete gc;
				break;
			}
			if (config.rootOnly) {
				terminationReason = "root_only";
				delete gc;
				break;
			}
		}

		delete gc;

	}

	worstLB = bestUB;
	if (config.rootOnly) {
		worstLB = rootLowerBound;
	} else {
		for(int s=0; s<arvore.size(); s++){
			if(arvore[s]->parentLB < worstLB){
				worstLB = arvore[s]->parentLB;
			}
		}
		if (interruptedLowerBound < worstLB) {
			worstLB = interruptedLowerBound;
		}
	}
	tempoExecucao = get_time() - globalStart;

	ofs << "Best Integer: " << bestUB << endl;
	ofs << "Lower Bound: " << worstLB << endl;
	ofs << "GAP: " << 100*(1 - worstLB/bestUB) << endl;
	ofs << "Time: " << tempoExecucao << endl;
	ofs << "Termination: " << terminationReason << endl;
	ofs << "Nodes Processed: " << processedNodes << endl;
	ofs << "CG Iterations: " << totalCgIterations << endl;
	ofs << "Generated Columns: " << totalGeneratedColumns << endl;
	ofs << "Duplicate Columns: " << totalDuplicateColumns << endl;
	ofs << "BEGIN_SUMMARY" << endl;
	ofs << "method=" << (useCuts ? "bcp" : "bp") << endl;
	ofs << "status=" << terminationReason << endl;
	ofs << "requests=" << requests.size() << endl;
	ofs << "time_limit_seconds=" << config.globalTimeLimitSeconds << endl;
	ofs << "elapsed_seconds=" << tempoExecucao << endl;
	ofs << "objective=" << bestUB << endl;
	ofs << "lower_bound=" << worstLB << endl;
	ofs << "gap_percent=" << 100*(1 - worstLB/bestUB) << endl;
	ofs << "nodes=" << processedNodes << endl;
	ofs << "cg_iterations=" << totalCgIterations << endl;
	ofs << "generated_columns=" << totalGeneratedColumns << endl;
	ofs << "duplicate_columns=" << totalDuplicateColumns << endl;
	ofs << "root_only=" << (config.rootOnly ? 1 : 0) << endl;
	ofs << "heuristic_time_limit_seconds=" << config.heuristicTimeLimitSeconds << endl;
	ofs << "restricted_mip_time_limit_seconds="
		<< config.restrictedMipTimeLimitSeconds << endl;
	ofs << "branching=most_fractional" << endl;
	ofs << "END_SUMMARY" << endl;
	ofs << "FINISHED!" << endl;

	ofs.close();
	delete [] redeAceita;
	for (GC *pending : arvore) {
		delete pending;
	}
}
