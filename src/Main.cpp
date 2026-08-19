#include "GC.h"
#include "BP.h"
#include "Request.h"
#include "ILPModel.h"
#include "Heuristica.h"
#include "Column.h"

#include <stdio.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>


using namespace std;

Graph * substrate = nullptr;
std::vector<Request*> requests;

bool readSubstrate(const char * subGraph) {
        
	FILE * arquivo = fopen(subGraph, "r");

	if (!arquivo){
		cerr << "Erro ao abrir arquivo de substrato '" << subGraph << "'." << endl;
		return false;
	}

	int n, m, k, l;
	int x, y;
	double cpu, banda, atraso;

	fscanf(arquivo, "%d %d", &n, &m);

	substrate = new Graph(n, m);


	for (int i = 0; i < substrate->getN(); i++) {
		fscanf(arquivo, "%d %d %lf", &x, &y, &cpu);
		
		substrate->addNode(Node(i, x, y, cpu));
    }

    
    for (int i = 0; i < substrate->getM(); i++) {
        fscanf(arquivo, "%d %d %lf %lf", &k, &l, &banda, &atraso);

        substrate->addEdge(Edge(i, k, l, banda, atraso));
    }

	fclose(arquivo);
	return true;
}

bool readVNsFolder(const char * folder, int numberVNs) {

	requests = std::vector<Request*>();

	int n, m, split, chegada, duracao, topologia, raio;

	for (int v = 0; v < numberVNs; v++) {
		const std::filesystem::path file = std::filesystem::path(folder) /
			("req" + std::to_string(v) + ".txt");

		FILE * arquivo = fopen(file.string().c_str(), "r");

		if(!arquivo){
			cerr << "Erro ao abrir arquivo '" << file.string() << "'." << endl;
			for (Request *request : requests) {
				delete request;
			}
			requests.clear();
			return false;
		}

		fscanf(arquivo, "%d %d %d %d %d %d %d", &n, &m, &split, &chegada,
			&duracao, &topologia, &raio);

		Request * r = new Request(v, chegada, duracao, raio);

		Graph * g = new Graph(n, m);
	
		int k, l;
		int x, y;
		double cpu;
		double banda, atraso;

		for (int i = 0; i < g->getN(); i++) {
			fscanf(arquivo, "%d %d %lf", &x, &y, &cpu);
			
			g->addNode(Node(i, x, y, cpu));
	    }

	    for (int i = 0; i < g->getM(); i++) {
	        fscanf(arquivo, "%d %d %lf %lf", &k, &l, &banda, &atraso);

	        g->addEdge(Edge(i, k, l, banda, atraso));
	    }

		r->setGraph(g);

		requests.push_back(r);

		fclose(arquivo);

	}

	return true;

}

int main(int argc, char *argv[])
{
	if (argc < 4 || argc > 5) {
		cerr << "Uso: " << argv[0]
			 << " <substrato.txt> <pasta_requisicoes> <quantidade> [saida.txt]" << endl;
		return EXIT_FAILURE;
	}

	char *end = nullptr;
	const long numberVNs = std::strtol(argv[3], &end, 10);
	if (*argv[3] == '\0' || *end != '\0' || numberVNs <= 0) {
		cerr << "A quantidade de requisicoes deve ser um inteiro positivo." << endl;
		return EXIT_FAILURE;
	}

	if (!readSubstrate(argv[1])) {
		return EXIT_FAILURE;
	}
	if (!readVNsFolder(argv[2], static_cast<int>(numberVNs))) {
		delete substrate;
		substrate = nullptr;
		return EXIT_FAILURE;
	}

	substrate->setDist(substrate);
	for(int v=0; v<requests.size(); v++){
		requests[v]->getGraph()->setDist(substrate);
	}

//	ILPModel ilp;
//	ilp.Solve(substrate, requests, 0, 0, 0, 0);

	BP bp;
	bp.Solve(substrate, requests, true, false, false,
		argc == 5 ? argv[4] : "saida.txt");

	delete substrate;
	for(int v=0; v<requests.size(); v++){
		delete requests[v];
	}
	requests.clear();
	return EXIT_SUCCESS;
}
