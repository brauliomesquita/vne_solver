#include "BP.h"
#include "Branch.h"
#include "Heuristica.h"


void BP::Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, bool useCuts, const char * outputfile){

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
	cout << "Colunas Iniciais:" << endl;
	for(int s=0; s<solucaoInicial.size(); s++){
		cout << solucaoInicial[s].v << "\t" << solucaoInicial[s].kl << "\t\t";
		cout << solucaoInicial[s].custoFO << endl;
		redeAceita[solucaoInicial[s].v] = true;
		bestUB += solucaoInicial[s].custoFO;
	}
	for(int v=0; v<requests.size(); v++)
		bestUB += 10000 * (1 -redeAceita[v]);
		
	cout << "Solução Inicial: " << bestUB << endl;
	
	raiz->parentPool = solucaoInicial;

	arvore.push_back(raiz);

    const char separator = ' ';
    const int nameWidth = 15;
    double worstLB;
    double tempoExecucao = 0;

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

	ofs << left << setw(nameWidth) << setfill(separator) << "ID";
    ofs << left << setw(nameWidth) << setfill(separator) << "Node Obj";
    ofs << left << setw(nameWidth) << setfill(separator) << "Node Int";
    ofs << left << setw(nameWidth) << setfill(separator) << "Best Integer";
    ofs << left << setw(nameWidth) << setfill(separator) << "Min LB";
    ofs << left << setw(nameWidth) << setfill(separator) << "GAP (%)";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Cols";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. Cols";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# CPU Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# BW Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Y Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# NG Cuts";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. CPU";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. BW";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. Y";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Gen. NG";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Feas. Checks";
    ofs << left << setw(nameWidth) << setfill(separator) << "# Feas. Unk";
    ofs << left << setw(nameWidth) << setfill(separator) << "Relax Time";
    ofs << left << setw(nameWidth) << setfill(separator) << "MasterTime";
    ofs << left << setw(nameWidth) << setfill(separator) << "SubTime";
    ofs << left << setw(nameWidth) << setfill(separator) << "CutTime";
    ofs << left << setw(nameWidth) << setfill(separator) << "Total Time";
    ofs << endl;

	while(arvore.size() != 0){
		
		if(tempoExecucao >= 3600)
			break;

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
			gc->Solve(substrate, requests, location, delay, resilience, useCuts, &y, &branch, &saida);
			end = get_time();
			tempoExecucao += gc->tempoTotal;

			if(gc->ub < bestUB){
				bestUB = gc->ub;
			}

			cout << "Número de colunas geradas: " << gc->pool.size() << endl;
			cout << "Número de cover cuts: " << gc->nCuts << endl;
			cout << "  CPU: " << gc->nCpuCuts
				<< " | Banda: " << gc->nBandwidthCuts
				<< " | Aceitação y: " << gc->nAcceptanceCuts
				<< " | No-good: " << gc->nNoGoodCuts << endl;

			cout << "Custo da relaxação da raiz: " << gc->lb << endl;
			cout << "Custo da Heuristica primal na raiz: " << gc->ub << endl;
			cout << "Tempo Relaxação Raiz: " << gc->tempoTotal << endl;


			if(gc->lb < bestUB){
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

			//ofs << fixed << setprecision(4);
			ofs << left << setw(nameWidth) << setfill(separator) << gc->id;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->lb;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->ub;
		    cout << "Best UB\t" << bestUB << endl;
		    ofs << left << setw(nameWidth) << setfill(separator) << bestUB;
		    ofs << left << setw(nameWidth) << setfill(separator) << worstLB;
		    ofs << left << setw(nameWidth) << setfill(separator) << 100*(1 - worstLB/bestUB);
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nCols;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gCols;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nCpuCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nBandwidthCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nAcceptanceCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nNoGoodCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gCpuCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gBandwidthCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gAcceptanceCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->gNoGoodCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nFeasibilityChecks;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->nFeasibilityUnknown;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->tempoRelaxacao;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->tempoMaster;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->tempoSub;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->tempoCuts;
		    ofs << left << setw(nameWidth) << setfill(separator) << gc->tempoTotal;
		    ofs << left << setw(nameWidth) << setfill(separator) << (end - init);
		    ofs << endl;
		}

		delete gc;

	}

	worstLB = bestUB;
	for(int s=0; s<arvore.size(); s++){
		if(arvore[s]->parentLB < worstLB){
			worstLB = arvore[s]->parentLB;
		}
	}

	ofs << "Best Integer: " << bestUB << endl;
	ofs << "Lower Bound: " << worstLB << endl;
	ofs << "GAP: " << 100*(1 - worstLB/bestUB) << endl;
	ofs << "Time: " << tempoExecucao << endl;
	ofs << "FINISHED!" << endl;

	cout << "Best Solution: " << bestUB << endl;

	ofs.close();
	delete [] redeAceita;
	for (GC *pending : arvore) {
		delete pending;
	}
}
