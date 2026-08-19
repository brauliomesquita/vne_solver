#include "Pricing.h"

Pricing::Pricing()
{
}

void Pricing::SetCplexParameters()
{
	/* Limite de tempo */
	problem->setParam(IloCplex::TiLim, 3600.0);

	problem->setParam(IloCplex::PreInd, 0);
	problem->setParam(IloCplex::AggInd, 0);
	problem->setParam(IloCplex::HeurFreq, -1);
	problem->setParam(IloCplex::NodeSel, CPX_NODESEL_BESTBOUND);

	problem->setParam(IloCplex::FracCuts, -1);
	problem->setParam(IloCplex::LiftProjCuts, -1);
	problem->setParam(IloCplex::FlowCovers, -1);
	problem->setParam(IloCplex::GUBCovers, -1);
	problem->setParam(IloCplex::Covers, -1);

	problem->setParam(IloCplex::MIPDisplay, 0);
	problem->setParam(IloCplex::SimDisplay, 0);
	problem->setParam(IloCplex::SiftDisplay, 0);

	problem->setParam(IloCplex::Threads, 1);

}

void Pricing::Solve(Graph* substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, IloNumArray2 gamma, IloNumArray3 alpha, IloNumArray3 pi, IloNumArray beta, std::vector<Column>* colunas, std::vector<Column> forbidden, std::vector<Branch> branchs)
{
# pragma omp parallel for
	for (int v = 0; v < requests.size(); v++) {
		for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {

			int k = requests[v]->getGraph()->getEdges()[kl].getOrig();
			int l = requests[v]->getGraph()->getEdges()[kl].getDest();

			IloEnv subEnv;
			IloModel subModel(subEnv);
			IloObjective objective(subEnv);
			problem = new IloCplex(subEnv);

			SetCplexParameters();

			char var_name[256];

			x = IntVarMatrix(subEnv, substrate->getN());
			for (int i = 0; i < substrate->getN(); i++) {
				x[i] = IloIntVarArray(subEnv, substrate->getN());
				for (int j = 0; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) != -1) {
						sprintf(var_name, "x_%d_%d_%d_%d", v, kl, i, j);
						x[i][j] = IloIntVar(subEnv, 0, 1, var_name);
						subModel.add(x[i][j]);
					}
				}
			}

			zk = IloIntVarArray(subEnv, substrate->getN());
			for (int i = 0; i < substrate->getN(); i++) {

				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;

				sprintf(var_name, "z_%d_%d_%d", v, k, i);
				zk[i] = IloIntVar(subEnv, 0, 1, var_name);
				subModel.add(zk[i]);
			}

			zl = IloIntVarArray(subEnv, substrate->getN());
			for (int i = 0; i < substrate->getN(); i++) {

				if (location && requests[v]->getGraph()->getDist(l, i) > requests[v]->getMaxD())
					continue;

				sprintf(var_name, "z_%d_%d_%d", v, l, i);
				zl[i] = IloIntVar(subEnv, 0, 1, var_name);
				subModel.add(zl[i]);
			}

			for (int b = 0; b < branchs.size(); b++) {
				if (branchs[b].v != v || branchs[b].tipo_branch == 3)
					continue;

				if (branchs[b].tipo_branch == 1) {
					int k_ = branchs[b].x;
					int i_ = branchs[b].y;

					int valor = branchs[b].valor;

					if (k_ == k) {
						zk[i_].setBounds(valor, valor);
					}

					if (k_ == l) {
						zl[i_].setBounds(valor, valor);
					}
				}

				if (branchs[b].tipo_branch == 2) {
					int kl_ = branchs[b].x;
					if (kl_ != kl)
						continue;


					int ij = branchs[b].y;

					int valor = branchs[b].valor;

					int i = substrate->getEdges()[ij].getOrig();
					int j = substrate->getEdges()[ij].getDest();

					subModel.add(x[i][j] + x[j][i] == valor);

				}

			}

			objective = IloAdd(subModel, IloMinimize(subEnv));

			IloExpr obj(subEnv);

			for (int i = 0; i < substrate->getN(); i++) {
				for (int j = i; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) != -1) {
						obj += (substrate->getCost(substrate->getAdj(i, j)) - beta[substrate->getAdj(i, j)]) * requests[v]->getGraph()->getEdges()[kl].getBW() * x[i][j];

						obj += (substrate->getCost(substrate->getAdj(i, j)) - beta[substrate->getAdj(i, j)]) * requests[v]->getGraph()->getEdges()[kl].getBW() * x[j][i];
					}
				}
			}

			for (int i = 0; i < substrate->getN(); i++) {
				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;
				obj -= alpha[v][kl][i] * zk[i];
			}

			for (int i = 0; i < substrate->getN(); i++) {
				if (location && requests[v]->getGraph()->getDist(l, i) > requests[v]->getMaxD())
					continue;
				obj -= pi[v][kl][i] * zl[i];
			}

			obj -= gamma[v][kl];

			objective.setExpr(obj);
			obj.end();

			IloExpr expr6(subEnv);
			for (int i = 0; i < substrate->getN(); i++) {
				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;
				expr6 += zk[i];
			}
			subModel.add(expr6 == 1);

			IloExpr expr7(subEnv);
			for (int i = 0; i < substrate->getN(); i++) {
				if (location && requests[v]->getGraph()->getDist(l, i) > requests[v]->getMaxD())
					continue;
				expr7 += zl[i];
			}
			subModel.add(expr7 == 1);

			/* Restrição 3: diferentes nós virtuais de uma mesma rede não serão alocados no mesmo nó físico */
			for (int i = 0; i < substrate->getN(); i++) {
				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;
				if (location && requests[v]->getGraph()->getDist(l, i) > requests[v]->getMaxD())
					continue;
				subModel.add(zk[i] + zl[i] <= 1);
			}

			/* Restrição 7: Fluxo */
			for (int i = 0; i < substrate->getN(); i++) {
				IloExpr exprA(subEnv), exprB(subEnv);
				IloExpr exprC(subEnv), exprD(subEnv);

				for (int j = 0; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) == -1)
						continue;
					exprA += x[i][j];
				}

				for (int j = 0; j < substrate->getN(); j++) {
					if (substrate->getAdj(i, j) == -1)
						continue;
					exprB += x[j][i];
				}

				IloExpr exprL(subEnv);
				if (!location || requests[v]->getGraph()->getDist(k, i) <= requests[v]->getMaxD())
					exprC += zk[i];

				if (!location || requests[v]->getGraph()->getDist(l, i) <= requests[v]->getMaxD())
					exprD += zl[i];

				subModel.add(exprA - exprB == exprC - exprD);

			}

			try {
				for (int f = 0; f < forbidden.size(); f++) {
					int cont = 2;
					IloExpr expr(subEnv);

					if (forbidden[f].v != v || forbidden[f].kl != kl) {
						continue;
					}

					int k = forbidden[f].k;
					int l = forbidden[f].l;

					expr += zk[k] + zl[l];

					std::vector<Edge> fEdges = forbidden[f].getEdges();

					for (int e = 0; e < fEdges.size(); e++) {

						int i = fEdges[e].getOrig();
						int j = fEdges[e].getDest();

						expr += x[i][j];
						expr += x[j][i];

						cont++;
					}

					subModel.add(expr <= cont - 1);
				}
			}
			catch (...) {
				cout << "Erro foi aqui! " << endl;
			}

			problem->extract(subModel);

			if (problem->solve()) {

				if (problem->getObjValue() <= -0.01) {
					Column c(v, kl);

					double custo = 0;
					for (int i = 0; i < substrate->getN(); i++) {
						for (int j = i; j < substrate->getN(); j++) {
							if (substrate->getAdj(i, j) != -1) {
								if (problem->getIntValue(x[i][j]) == 1 || problem->getIntValue(x[j][i]) == 1) {

									c.addEdge(substrate->getEdges()[substrate->getAdj(i, j)]);
									custo += substrate->getCost(substrate->getAdj(i, j)) * requests[v]->getGraph()->getEdges()[kl].getBW();
								}
							}
						}
					}

					for (int i = 0; i < substrate->getN(); i++) {
						if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
							continue;
						if (problem->getIntValue(zk[i]) == 1)
							c.k = i;
					}


					for (int i = 0; i < substrate->getN(); i++) {
						if (location && requests[v]->getGraph()->getDist(l, i) > requests[v]->getMaxD())
							continue;
						if (problem->getIntValue(zl[i]) == 1)
							c.l = i;
					}

					c.custoFO = custo;
					colunas->push_back(c);
				}
			}

			delete problem;
			subEnv.end();

		}
	}

}
