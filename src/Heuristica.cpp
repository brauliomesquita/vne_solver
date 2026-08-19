#include "Heuristica.h"
#include <cstdlib>

void Heuristica::Construtiva(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, std::vector<Column>* solucao){
	Dijkstra dj;

	double *resCPU = new double[substrate->getN()];
	for(int i=0; i<substrate->getN(); i++){
		resCPU[i] = substrate->getNodes()[i].getCPU();
	}

	double ** resBand = new double*[substrate->getN()];
	for(int i=0; i<substrate->getN(); i++){
		resBand[i] = new double[substrate->getN()];
		for(int j=0; j<substrate->getN(); j++){
			if(substrate->getAdj(i, j) != -1)
				resBand[i][j] = substrate->getEdges()[substrate->getAdj(i, j)].getBW();
			else 
				resBand[i][j] = -1;
		}
	}
	
	// nodeMap[i] = j  => nó virtual j alocado ao nó físico i
	int *nodeMap = new int[substrate->getN()];
	double *LC = new double[substrate->getN()];

	srand(static_cast<unsigned int>(time(NULL)));

	// Para cada rede virtual
	for(int v=0; v<requests.size(); v++){
		std::vector<Column> colunasTemp;
	
		bool redeAceita = true;
		
		for(int i=0; i<substrate->getN(); i++){
			nodeMap[i] = -1;
		}
		
		// Para cada nó virtual
		for(int k=0; k<requests[v]->getGraph()->getN(); k++){
			// calcula lista de candidatos
			
			// Vejo se pode ser mapeado
			int cont = 0;
			for(int i=0; i<substrate->getN(); i++){
				if(location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD()){
					LC[i] = 0;
				} else if(resCPU[i] < requests[v]->getGraph()->getNodes()[k].getCPU()){
					LC[i] = 0;
				} else if(nodeMap[i] != -1){
					LC[i] = 0;
				} else {
					LC[i] = 1;
					cont++;
				}
			}

			// Faço um cálculo de quão bom o nó pode ser
			for(int i=0; i<substrate->getN(); i++){
				if(LC[i] == 1){
					double somaResBand = 0.0;
					for(int j=0; j<substrate->getN(); j++){
						if(substrate->getAdj(i, j) != -1){
							somaResBand += resBand[i][j];
						}
					}
					if(v == 1 && i==34)
						cout << ">>>\t\t" << resCPU[i] << "\t" << somaResBand << endl;
					LC[i] = resCPU[i] * somaResBand;
				}
			}
					
			// escolhe nó físico da lista
			int i_ = -1;
			
			double maior = 0.001;
			for(int i=0; i<substrate->getN(); i++){
				if(LC[i])
					cout << "LC_" << i << ": " << LC[i] << endl;
				if(LC[i] > maior){
					i_ = i;
					maior = LC[i];
				}
			}
			
			if(i_ == -1){
				// rejeita a rede
				redeAceita = false;
				break;
			} else {
				// mapeia o nó
				nodeMap[i_] = k;
			}
			
		}
		
		// Rede rejeitada => pula para a próxima
		if(!redeAceita){
			//cout << "A rede " << v << " foi REJEITADA!" << endl;
			continue;
		}
		
		int * map = new int[requests[v]->getGraph()->getN()];
		
		// Caso a rede seja aceita
		cout << endl;
		for(int i=0; i<substrate->getN(); i++){
			if(nodeMap[i] == -1)
				continue;
			int k = nodeMap[i];
			cout << "\t Para k=" << k << ", escolheu o nó físico " << i << endl;
			map[k] = i;
		}
	
		for(int m=0; m<requests[v]->getGraph()->getM(); m++){
			Edge e = requests[v]->getGraph()->getEdges()[m];
			int orig = e.getOrig();
			int dest = e.getDest();
			
			Column col(v, m);
			col.k = map[orig];
			col.l = map[dest];
			double custoFO = 0.0;
			
			// Encontrar caminho entre map[orig] e map[dest];
			std::vector<int> caminho = dj.Run(resBand, substrate, map[orig], map[dest], e.getBW());
			
			if(caminho.size() > 1){
				for(int c=0; c<caminho.size()-1; c++){	
					col.addEdge(substrate->getEdges()[substrate->getAdj(caminho[c], caminho[c+1])]);
					custoFO += substrate->getCost(substrate->getAdj(caminho[c], caminho[c+1])) * e.getBW();
					
					resBand[caminho[c]][caminho[c+1]] -= e.getBW();
					resBand[caminho[c+1]][caminho[c]] -= e.getBW();
				}
			} else {

				redeAceita = false;
				break;
			}
			
			col.custoFO = custoFO;
			colunasTemp.push_back(col);				
		}
		
		if(redeAceita){
			cout << "Aceita!" << endl;

			for(int i=0; i<substrate->getN(); i++){
				if(nodeMap[i] == -1)
					continue;
				int k = nodeMap[i];
				resCPU[i] -= requests[v]->getGraph()->getNodes()[k].getCPU();
			}

			solucao->insert(solucao->end(), colunasTemp.begin(), colunasTemp.end());
		} else {

			// Voltar banda residual
			for(int t=0; t<colunasTemp.size(); t++){
				int kl = colunasTemp[t].kl;
				std::vector<Edge> edges = colunasTemp[t].getEdges();
				for(int e=0; e<edges.size(); e++){
					int i = edges[e].getOrig();
					int j = edges[e].getDest();

					resBand[i][j] += requests[v]->getGraph()->getEdges()[kl].getBW();
					resBand[j][i] += requests[v]->getGraph()->getEdges()[kl].getBW();
				}
			}
			colunasTemp.clear();
		}
		
		colunasTemp.clear();
		
		delete [] map;
	}

	for (int i = 0; i < substrate->getN(); ++i) {
		delete [] resBand[i];
	}
	delete [] resBand;
	delete [] resCPU;
	delete [] nodeMap;
	delete [] LC;
}
