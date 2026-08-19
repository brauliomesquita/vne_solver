#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

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
#include <string>

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
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	if (argc < 5) {
		cerr << "Uso: " << argv[0]
			 << " <bp|bcp|ilp> <substrato.txt> <pasta_requisicoes> <quantidade>"
			 << " [saida.txt] [--time-limit segundos]"
			 << " [--heuristic-time-limit segundos]"
			 << " [--restricted-mip-time-limit segundos] [--root-only]" << endl;
		return EXIT_FAILURE;
	}

	const std::string method = argv[1];
	if (method != "bp" && method != "bcp" && method != "ilp") {
		cerr << "Metodo de resolucao invalido: '" << method
			 << "'. Use 'bp', 'bcp' ou 'ilp'." << endl;
		return EXIT_FAILURE;
	}

	char *end = nullptr;
	const long numberVNs = std::strtol(argv[4], &end, 10);
	if (*argv[4] == '\0' || *end != '\0' || numberVNs <= 0) {
		cerr << "A quantidade de requisicoes deve ser um inteiro positivo." << endl;
		return EXIT_FAILURE;
	}

	SolverConfig config;
	std::string outputfile = method == "ilp" ? "saida-ilp.txt" : "saida.txt";
	int argument = 5;
	if (argument < argc && std::string(argv[argument]).rfind("--", 0) != 0) {
		outputfile = argv[argument++];
	}
	auto parseNonNegative = [&](const char *option, double *destination) -> bool {
		if (argument >= argc) {
			cerr << "Falta o valor de " << option << "." << endl;
			return false;
		}
		char *valueEnd = nullptr;
		const double value = std::strtod(argv[argument++], &valueEnd);
		if (*valueEnd != '\0' || value < 0.0) {
			cerr << "Valor invalido para " << option << "." << endl;
			return false;
		}
		*destination = value;
		return true;
	};
	while (argument < argc) {
		const std::string option = argv[argument++];
		if (option == "--root-only") {
			config.rootOnly = true;
		} else if (option == "--time-limit") {
			if (!parseNonNegative("--time-limit", &config.globalTimeLimitSeconds))
				return EXIT_FAILURE;
		} else if (option == "--heuristic-time-limit") {
			if (!parseNonNegative("--heuristic-time-limit",
				&config.heuristicTimeLimitSeconds)) return EXIT_FAILURE;
		} else if (option == "--restricted-mip-time-limit") {
			if (!parseNonNegative("--restricted-mip-time-limit",
				&config.restrictedMipTimeLimitSeconds)) return EXIT_FAILURE;
		} else {
			cerr << "Opcao desconhecida: " << option << endl;
			return EXIT_FAILURE;
		}
	}
	if (config.globalTimeLimitSeconds <= 0.0) {
		cerr << "--time-limit deve ser maior que zero." << endl;
		return EXIT_FAILURE;
	}
	if (method == "ilp" && config.rootOnly) {
		cerr << "--root-only e valido apenas para bp e bcp." << endl;
		return EXIT_FAILURE;
	}

	if (!readSubstrate(argv[2])) {
		return EXIT_FAILURE;
	}
	if (!readVNsFolder(argv[3], static_cast<int>(numberVNs))) {
		delete substrate;
		substrate = nullptr;
		return EXIT_FAILURE;
	}

	substrate->setDist(substrate);
	for(int v=0; v<requests.size(); v++){
		requests[v]->getGraph()->setDist(substrate);
	}
	if (method == "bp" || method == "bcp") {
		BP bp;
		bp.Solve(substrate, requests, false, false, false,
			method == "bcp", outputfile.c_str(), config);
	} else {
		ILPModel ilp;
		const double objective = ilp.Solve(
			substrate, requests, false, false, false, 1, config,
			outputfile.c_str());
		cout << "Valor objetivo ILP: " << objective << endl;
	}

	delete substrate;
	for(int v=0; v<requests.size(); v++){
		delete requests[v];
	}
	requests.clear();
	return EXIT_SUCCESS;
}
