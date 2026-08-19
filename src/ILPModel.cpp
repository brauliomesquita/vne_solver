#include "ILPModel.h"
#include "Utility.h"

 ILPModel::ILPModel() {
	 relaxacao = false;
 }

 void ILPModel::SetCplexParameters() {
	/* Limite de tempo */
	problem->setParam(IloCplex::TiLim, 3600.0);

 	problem->setParam(IloCplex::PreInd, 0);
 	problem->setParam(IloCplex::AggInd, 0);
 	problem->setParam(IloCplex::HeurFreq, -1);

 	problem->setParam(IloCplex::NodeSel, CPX_NODESEL_DFS);
 	problem->setParam(IloCplex::NodeSel, CPX_NODESEL_BESTBOUND);

 	problem->setParam(IloCplex::FracCuts, -1);
 	problem->setParam(IloCplex::LiftProjCuts, -1);
 	problem->setParam(IloCplex::FlowCovers, -1);
 	problem->setParam(IloCplex::GUBCovers, -1);
 	problem->setParam(IloCplex::Covers, -1);

 	problem->setParam(IloCplex::ZeroHalfCuts, -1);
 	problem->setParam(IloCplex::ImplBd, -1);
 	problem->setParam(IloCplex::Cliques, -1);
 	problem->setParam(IloCplex::DisjCuts, -1);
 	problem->setParam(IloCplex::FlowPaths, -1);
 	problem->setParam(IloCplex::MIRCuts, -1);

 }

 /* FO -- 1: padrão (banda), 2: alfa, 3: delay */
 double ILPModel::Solve(Graph *substrate, std::vector<Request*> requests,
	 bool location, bool delay, bool resilience, int fo,
	 const SolverConfig& config, const char *outputfile) {
	const double start = get_time();

 	IloEnv env;
 	IloModel model(env);
 	IloObjective objective(env);
 	problem = new IloCplex(env);
	problem->setOut(env.getNullStream());
	problem->setWarning(env.getNullStream());

 	problem->out() << "Result Compact Model  " << endl;
 	problem->out() << "Substrate Size:       " << substrate->getN() << endl;
 	problem->out() << "Number os VNs:        " << requests.size() << endl;
 	problem->out() << "Parameters: " << endl;
 	if(location)
 		problem->out() << "Location" << endl;
 	if(delay)
 		problem->out() << "Delay" << endl;
 	if(resilience){
 		problem->out() << "Resilience" << endl;
 		//P = 2;
 	}
 	problem->out() << "Objective Function:" << endl;
 	if(fo == 1)
 		problem->out() << "Minimize Band" << endl;
 	if(fo == 2)
 		problem->out() << "Load Balance" << endl;
 	if(fo == 3)
 		problem->out() << "Minimize Delay" << endl;
 	problem->out() << endl << endl;
 	
	//SetCplexParameters();
	//problem->setParam(IloCplex::Threads, 1);
	problem->setParam(IloCplex::TiLim, config.globalTimeLimitSeconds);

 	x = IntVar4Matrix(env, requests.size());
 	z = IntVar3Matrix(env, requests.size());
 	y = IloIntVarArray(env, requests.size());

 	char var_name[256];

 	for (int v = 0; v < requests.size(); v++) {
 		x[v] = IntVar3Matrix(env, requests[v]->getGraph()->getM());

 		for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
 			x[v][kl] = IntVarMatrix(env, substrate->getN());

 			for(int i = 0; i<substrate->getN(); i++){
 				x[v][kl][i] = IloIntVarArray(env, substrate->getN());

				for(int j = 0; j<substrate->getN(); j++){
					if(substrate->getAdj(i, j)!=-1){

						sprintf(var_name, "x_%d_%d_%d_%d", v, kl, i, j);
						x[v][kl][i][j] = IloIntVar(env, 0, 1, var_name);
 			 			model.add(x[v][kl][i][j]);

					}
				}
 			}
 		}
 	}

 	for (int v = 0; v < requests.size(); v++) {
 		z[v] = IntVarMatrix(env, requests[v]->getGraph()->getN());
 		for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
 			z[v][k] = IloIntVarArray(env, substrate->getN());
 			for (int i = 0; i < substrate->getN(); i++) {

 				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
 					continue;

 				sprintf(var_name, "z_%d_%d_%d", v, k, i);
 				z[v][k][i] = IloIntVar(env, 0, 1, var_name);
 				model.add(z[v][k][i]);
 			}
 		}
 	}

	for (int v = 0; v < requests.size(); v++) {
		sprintf(var_name, "y_%d", v);
		y[v] = IloIntVar(env, 0, 1, var_name);
		model.add(y[v]);
	}

 	objective = IloAdd(model, IloMinimize(env));

 	 IloExpr obj(env);

 	
 	for (int v = 0; v < requests.size(); v++) {
 		for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
 			for(int i = 0; i<substrate->getN(); i++){
				for(int j = 0; j<substrate->getN(); j++){
					if(substrate->getAdj(i, j)!=-1){
 							obj += substrate->getCost(substrate->getAdj(i, j)) * requests[v]->getGraph()->getEdges()[kl].getBW() * x[v][kl][i][j];
					}
				
				}
 			
 			}
 		}
 	}

 // 	if(fo == 2){

 // 		IloNumVar alpha = IloNumVar(env, 0, IloInfinity, "alpha");
 // 		model.add(alpha);

 // 		obj += alpha;

 // 		for (int i = 0; i < substrato->n; i++) {
 // 			for (int j = 0; j < substrato->n; j++) {
 // 				if (substrato->banda[i][j] == 0)
 // 					continue;

 // 				IloExpr expr(env);
 				
 // 				for (int v = 0; v < requests.size(); v++) {
 // 					for (int k = 0; k < requisicoes[v]->n; k++) {
 // 						for (int l = 0; l < requisicoes[v]->n; l++) {
 // 							if (requisicoes[v]->banda[k][l] == 0)
 // 								continue;

 // 							expr += requisicoes[v]->banda[k][l] * x[v][k][l][i][j];

 // 						}
 // 					}
 // 				}

 // 				model.add(alpha >= expr);

 // 			}
 // 		}

 // 	}

	 	for (int v = 0; v < requests.size(); v++) {
	 		obj += M * (1 - y[v]);
	 	}

	 	objective.setExpr(obj);
	 	obj.end();

	for (int v = 0; v < requests.size(); v++) {
		for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
			IloExpr expr6(env);

			for (int i = 0; i < substrate->getN(); i++) {

				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;

				expr6 += z[v][k][i];
			}

			model.add(expr6 - y[v] == 0);
		}
	}

	/* Restrição 3: diferentes nós virtuais de uma mesma rede não serão alocados no mesmo nó físico */
 	for (int v = 0; v < requests.size(); v++) {
 		for (int i = 0; i < substrate->getN(); i++) {
 			IloExpr expr7(env);
 			bool flag = false;
 			for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {

 				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
 					continue;

 				flag = true;
 				expr7 += z[v][k][i];
 			}

 			if (flag)
 				model.add(expr7 <= 1);
 		}
 	}

	/* Restrição 4: CPU */
	for (int i = 0; i < substrate->getN(); i++) {
		IloExpr expr4(env);
		bool flag = false;
		for (int v = 0; v < requests.size(); v++) {
			for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {

				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;
				flag = true;
				expr4 += requests[v]->getGraph()->getNodes()[k].getCPU() * z[v][k][i];
			}
		}
		if (flag)
			model.add(expr4 - substrate->getNodes()[i].getCPU() <= 0);
	}

	// /* Restrição 6: Banda */
 	for (int i = 0; i < substrate->getN(); i++) {
 		for (int j = 0; j < substrate->getN(); j++) {

 			if (substrate->getAdj(i,j) == -1)
 				continue;

 			IloExpr expr5(env);
 			bool flag = false;
 			for (int v = 0; v < requests.size(); v++) {
 				for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
						flag = true;

 						expr5 += requests[v]->getGraph()->getEdges()[kl].getBW() * x[v][kl][i][j];
 						expr5 += requests[v]->getGraph()->getEdges()[kl].getBW() * x[v][kl][j][i];

 				}
 			}
 			if(flag)
 				model.add(expr5 - substrate->getEdges()[substrate->getAdj(i, j)].getBW() <= 0);
 		}
 	}

 // 	if(delay){
 // 		for (int v = 0; v < requests.size(); v++) {
 // 			for (int k = 0; k < requisicoes[v]->n; k++) {
 // 				for (int l = 0; l < requisicoes[v]->n; l++) {

 // 					if (requisicoes[v]->banda[k][l] == 0)
 // 						continue;

 // 					IloExpr expr5(env);

 // 					bool flag = false;
 // 					for (int i = 0; i < substrato->n; i++) {
 // 						for (int j = 0; j < substrato->n; j++) {

 // 							if (substrato->banda[i][j] == 0)
 // 								continue;

 // 							flag = true;
 // 							expr5 += substrato->atraso[i][j] * x[v][k][l][i][j];
 // 							expr5 += substrato->atraso[i][j] * x[v][k][l][j][i];

 // 						}
 // 					}

 // 					if(flag)
 // 						model.add(expr5 <= P * requisicoes[v]->atraso[k][l]);
 // 				}
 // 			}
 // 		}
 // 	}

	// /* Restrição 7: Fluxo */
 	for (int v = 0; v < requests.size(); v++) {
 		for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
			int k = requests[v]->getGraph()->getEdges()[kl].getOrig();
			int l = requests[v]->getGraph()->getEdges()[kl].getDest();

			for (int i = 0; i < substrate->getN(); i++) {

				IloExpr exprA(env), exprB(env);
				IloExpr exprC(env), exprD(env);

				for (int j = 0; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) == -1)
						continue;
					exprA += x[v][kl][i][j];
				}

				for (int j = 0; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) == -1)
						continue;
					exprB += x[v][kl][j][i];
				}

				IloExpr exprL(env);
				if (!location || requests[v]->getGraph()->getDist(k, i) <= requests[v]->getMaxD())
					exprC += z[v][k][i];

				if (!location || requests[v]->getGraph()->getDist(l, i) <= requests[v]->getMaxD())
					exprD += z[v][l][i];

				model.add(exprA - exprB == exprC - exprD);

			}

		

 		}
 	}

  	problem->extract(model);
  	problem->exportModel("modeloCompacto.lp");

	// // RELAXAÇÃO
 // 	if(this->relaxacao){
 // 		//SetCplexParameters();
 // 		for (int v = 0; v < requests.size(); v++) {
 // 			for (int k = 0; k < requisicoes[v]->n; k++) {
 // 				for (int l = 0; l < requisicoes[v]->n; l++) {
 // 					if (requisicoes[v]->banda[k][l] == 0)
 // 						continue;
 // 					for (int i = 0; i < substrato->n; i++) {
 // 						for (int j = 0; j < substrato->n; j++) {
 // 							if (substrato->banda[i][j] == 0)
 // 								continue;
 // 							model.add(IloConversion(env, x[v][k][l][i][j], ILOFLOAT));
 // 						}
 // 					}
 // 				}
 // 			}
 // 		}

 // 		for (int v = 0; v < requests.size(); v++) {
 // 			for (int k = 0; k < requisicoes[v]->n; k++) {
 // 				for (int i = 0; i < substrato->n; i++) {
 // 					if (location
 // 						&& !requisicoes[v]->mapping[k][i])
 // 						continue;
 // 					model.add(IloConversion(env, z[v][k][i], ILOFLOAT));
 // 				}
 // 			}
 // 		}

 // 		for (int v = 0; v < requests.size(); v++) {
 // 			model.add(IloConversion(env, y[v], ILOFLOAT));
 // 		}
 // 	}

	// float accRatio = 0;
	bool solved = false;
	try {
		solved = problem->solve();
	} catch (IloException& e) {
		cerr << "ERROR: " << e.getMessage() << endl;
	} catch (...) {
		cerr << "Error" << endl;
	}
 // 		end_ = get_time();

 // 		gap = problem->getMIPRelativeGap() * 100.0;
 		
 // 		float contador = 0;
 // 		for (int v = 0; v < requisicoes.size(); v++) {
 // 			contador += problem->getIntValue(y[v]);
 // 		}
 // 		accRatio = contador/requisicoes.size();

 // 		for (int v = 0; v < requisicoes.size(); v++) {
 // 			for(int k=0; k<requisicoes[v]->n; k++){
 // 				for(int i=0; i<substrato->n; i++){
 // 					if(!requisicoes[v]->mapping[k][i])
 // 						continue;
 // 					float temp = problem->getValue(z[v][k][i]);
	// 				//problem->out() << z[v][k][i].getName() << " = " << problem->getIntValue(z[v][k][i]) << endl;
 // 				}
 // 			}
 // 		}

 // 		for (int i = 0; i < substrato->n; i++) {
 // 			for (int j = 0; j < substrato->n; j++) {

 // 				if (substrato->banda[i][j] == 0)
 // 					continue;
 // 				float sum = 0;
 // 				for (int v = 0; v < requests.size(); v++) {
 // 					for (int k = 0; k < requisicoes[v]->n; k++){
 // 						for (int l = 0; l < requisicoes[v]->n; l++) {
 // 							if (requisicoes[v]->banda[k][l] == 0)
 // 								continue;

 // 							//problem->out() << x[v][k][l][i][j].getName()  << " = " <<  problem->getIntValue(x[v][k][l][i][j]) << endl;

 // 						}
 // 					}
 // 				}
 // 			}
 // 		}

 // 		problem->out() << endl << "Banda Utilizada" << endl << endl;

 // 		for (int i = 0; i < substrato->n; i++) {
 // 			for (int j = 0; j < substrato->n; j++) {

 // 				if (substrato->banda[i][j] == 0)
 // 					continue;
 // 				float sum = 0;
 // 				for (int v = 0; v < requests.size(); v++) {
 // 					for (int k = 0; k < requisicoes[v]->n; k++){
 // 						for (int l = 0; l < requisicoes[v]->n; l++) {
 // 							if (requisicoes[v]->banda[k][l] == 0)
 // 								continue;

 // 							sum += requisicoes[v]->banda[k][l] * problem->getIntValue(x[v][k][l][i][j]);

 // 						}
 // 					}
 // 				}
 // 				problem->out() << i << " " << j << " " << sum << " " << substrato->banda[i][j] << endl;
 // 			}
 // 		}

 // 		problem->out() << endl << "CPU Utilizada" << endl << endl;

 // 		for (int i = 0; i < substrato->n; i++) {
 // 			float sum = 0;
 // 			for (int v = 0; v < requests.size(); v++) {
 // 				for (int k = 0; k < requisicoes[v]->n; k++){
 // 					if (location && !requisicoes[v]->mapping[k][i])
 // 						continue;
 // 					sum += requisicoes[v]->cpu[k] * problem->getIntValue(z[v][k][i]);
 // 				}
 // 			}
 // 			problem->out() << i << " " << sum << " " << substrato->cpu[i] << endl;
 // 		}

 		
 // 			} catch (IloException& e) {
 // 				cerr << "ERROR: " << e.getMessage() << endl;
 // 			} catch (...) {
 // 				cerr << "Error" << endl;
 // 			}
 // 			float custoSolucao = problem->getObjValue();

 // 			problem->out() << endl << endl;

 // 			problem->out() << "Solution Cost:    " << custoSolucao << endl;
 // 			problem->out() << "Optm GAP:         " << gap << endl;
 // 			problem->out() << "Acceptance Ratio: " << accRatio << endl;
 // 			problem->out() << "Time:             " << end_ - init_ << endl;

 // 			delete problem;
 // 			env.end();

	const double elapsed = get_time() - start;
	double objectiveValue = VNE_INFINITY;
	double lowerBound = 0.0;
	double gapPercent = 100.0;
	std::string status = "not_solved";
	if (solved) {
		objectiveValue = problem->getObjValue();
		lowerBound = problem->getBestObjValue();
		gapPercent = problem->getMIPRelativeGap() * 100.0;
		status = problem->getStatus() == IloAlgorithm::Optimal ?
			"optimal" : "time_limit_or_feasible";
	} else if (problem->getStatus() == IloAlgorithm::Infeasible) {
		status = "infeasible";
	}

	std::ofstream result(outputfile, std::ofstream::out);
	result << "ILP Result" << endl;
	result << "Objective: " << objectiveValue << endl;
	result << "Lower Bound: " << lowerBound << endl;
	result << "GAP: " << gapPercent << endl;
	result << "Time: " << elapsed << endl;
	result << "Status: " << status << endl;
	result << "BEGIN_SUMMARY" << endl;
	result << "method=ilp" << endl;
	result << "status=" << status << endl;
	result << "requests=" << requests.size() << endl;
	result << "time_limit_seconds=" << config.globalTimeLimitSeconds << endl;
	result << "elapsed_seconds=" << elapsed << endl;
	result << "objective=" << objectiveValue << endl;
	result << "lower_bound=" << lowerBound << endl;
	result << "gap_percent=" << gapPercent << endl;
	result << "nodes=0" << endl;
	result << "cg_iterations=0" << endl;
	result << "generated_columns=0" << endl;
	result << "duplicate_columns=0" << endl;
	result << "root_only=0" << endl;
	result << "heuristic_time_limit_seconds=0" << endl;
	result << "restricted_mip_time_limit_seconds=0" << endl;
	result << "tree_threads=0" << endl;
	result << "branching=cplex" << endl;
	result << "END_SUMMARY" << endl;
	result.close();

	delete problem;
	env.end();
	return objectiveValue;

 		}
