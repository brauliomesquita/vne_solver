#include "GC.h"

#include <algorithm>

GC::GC(){
	id = 1;
	parentLB = 0.0;
	lb = 0.0;
	ub = VNE_INFINITY;
	sol_inteira = true;
	nCols = 0;
	gCols = 0;
	nCuts = 0;
	gCuts = 0;

	pool = std::vector<Column>();
	parentPool = std::vector<Column>();
	forbidden = std::vector<Column>();
	branchs = std::vector<Branch>();
	cpuCoverCuts = std::vector<CpuCoverCut>();
}

GC::GC(GC * parent){
	id = 0;
	lb = parent->lb;
	ub = VNE_INFINITY;
	sol_inteira = true;
	nCols = 0;
	gCols = 0;
	nCuts = static_cast<unsigned int>(parent->cpuCoverCuts.size());
	gCuts = 0;
	this->pool = std::vector<Column>();
	this->parentPool = std::vector<Column>(parent->pool);
	this->branchs = std::vector<Branch>(parent->branchs);
	this->forbidden = std::vector<Column>(parent->forbidden);
	this->cpuCoverCuts = std::vector<CpuCoverCut>(parent->cpuCoverCuts);
	this->parentLB = parent->lb;
}

void GC::addBranchLambda(int m, int valor){

	this->parentPool[m].lowerBound = this->parentPool[m].upperBound = valor;

	if(valor == 0){
		this->forbidden.push_back(parentPool[m]);
	}
	
}

void GC::addBranch(Branch branch, int valor){
	branch.valor = valor;
	this->branchs.push_back(branch);

	// Se branch_tipo == 2, eliminar colunas
	if(branch.tipo_branch == 2){
		int v = branch.v;
		int kl = branch.x;
		int ij = branch.y;

		int valor = branch.valor;

		for(int p=0; p<parentPool.size(); p++){
			
			if(parentPool[p].v != v || parentPool[p].kl != kl){
				continue;
			}

			bool flag = false;
			std::vector<Edge> edges = parentPool[p].getEdges();
			for(int e=0; e<edges.size(); e++){
				if(ij == edges[e].getId()){
					flag = true;
				}
			}
			
			if(flag != (valor != 0)){
				parentPool[p].lowerBound = parentPool[p].upperBound = 0;
			}
						
		}

	}

}

void GC::SetCplexParameters() {
	master->setParam(IloCplex::TiLim, 3600.0);

	/*master->setParam(IloCplex::PreInd, 0);
	master->setParam(IloCplex::AggInd, 0);
	master->setParam(IloCplex::HeurFreq, -1);

	master->setParam(IloCplex::FracCuts, -1);
	master->setParam(IloCplex::LiftProjCuts, -1);
	master->setParam(IloCplex::FlowCovers, -1);
	master->setParam(IloCplex::GUBCovers, -1);
	master->setParam(IloCplex::Covers, -1);

	master->setParam(IloCplex::ZeroHalfCuts, -1);
	master->setParam(IloCplex::ImplBd, -1);
	master->setParam(IloCplex::Cliques, -1);
	master->setParam(IloCplex::DisjCuts, -1);
	master->setParam(IloCplex::FlowPaths, -1);
	master->setParam(IloCplex::MIRCuts, -1);
*/
	master->setParam(IloCplex::MIPDisplay, 0);
	master->setParam(IloCplex::SimDisplay, 0);
	master->setParam(IloCplex::SiftDisplay, 0);

	//master->setParam(IloCplex::Threads, 1);
}


void GC::CreateVariables(){
	y = IloNumVarArray(env, requests.size());
	lambda = IloNumVarArray(env);

	char var_name[256];

	for (int v = 0; v < requests.size(); v++) {
		sprintf(var_name, "y_%d", v);
		y[v] = IloNumVar(env, 0, 1, var_name);
		model.add(y[v]);
	}

	z = NumVar3Matrix(env, requests.size());
	for (int v = 0; v < requests.size(); v++) {
		z[v] = NumVarMatrix(env, requests[v]->getGraph()->getN());
		for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
			z[v][k] = IloNumVarArray(env, substrate->getN());
			for (int i = 0; i < substrate->getN(); i++) {

				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;

				sprintf(var_name, "z_%d_%d_%d", v, k, i);
				z[v][k][i] = IloNumVar(env, 0, 1, var_name);
				model.add(z[v][k][i]);
			}
		}
	}
}

void GC::CreateObjectiveFunction() {

	objective = IloAdd(model, IloMinimize(env));

	IloExpr obj(env);

	for (int v = 0; v < requests.size(); v++) {
		obj += M * (1 - y[v]);
	}

	objective.setExpr(obj);
	obj.end();

}

void GC::CreateConstraints() {
	constraint_cpu_cover = OneDimRange(env);

	for (int v = 0; v < requests.size(); v++) {
		for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
			IloExpr expr6(env);

			for (int i = 0; i < substrate->getN(); i++) {

				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
					continue;

				expr6 += z[v][k][i];
			}

			model.add(expr6 <= y[v]);
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

	for (const CpuCoverCut& cut : cpuCoverCuts) {
		addCpuCoverCutToModel(cut);
	}

	char cName[256];

	constraint_lambda = TwoDimRange(env, requests.size());
	for (int v = 0; v < requests.size(); v++) {
		constraint_lambda[v] = OneDimRange(env, requests[v]->getGraph()->getM());
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			sprintf(cName, "constLambda_%d_%d", v, kl);
			constraint_lambda[v][kl] = -y[v] == 0;
			constraint_lambda[v][kl].setName(cName);
		}
		model.add(constraint_lambda[v]);
	}
	
	// Bandwidth constraints
	constraint_bw = OneDimRange(env, substrate->getM());
	for (int i = 0; i < substrate->getM(); i++) {
		for (int j = i; j < substrate->getN(); j++) {
			if(substrate->getAdj(i,j) != -1){
				IloExpr expr_band(env);
				constraint_bw[substrate->getAdj(i,j)] = expr_band <= substrate->getEdges()[substrate->getAdj(i,j)].getBW();
				sprintf(cName, "constBW_%d_%d", i, j);
				constraint_bw[substrate->getAdj(i,j)].setName(cName);
			}
		}
	}
	model.add(constraint_bw);

		constraint_saida = ThreeDimRange(env, requests.size());
	for (int v = 0; v < requests.size(); v++) {
		constraint_saida[v] = TwoDimRange(env, requests[v]->getGraph()->getM());
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			constraint_saida[v][kl] = OneDimRange(env, substrate->getN());
			int k = requests[v]->getGraph()->getEdges()[kl].getOrig();
			for(int i = 0; i < substrate->getN(); i++){			
				if(!location || requests[v]->getGraph()->getDist(k, i) <= requests[v]->getMaxD()){
					sprintf(cName, "constSaida_%d_%d_%d", v, kl, i);

					constraint_saida[v][kl][i] = -z[v][k][i] <= 0;
					model.add(constraint_saida[v][kl][i]);
					constraint_saida[v][kl][i].setName(cName);
				}
			}
		}
	}

	constraint_entrada = ThreeDimRange(env, requests.size());
	for (int v = 0; v < requests.size(); v++) {
		constraint_entrada[v] = TwoDimRange(env, requests[v]->getGraph()->getM());
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			constraint_entrada[v][kl] = OneDimRange(env, substrate->getN());
			int l = requests[v]->getGraph()->getEdges()[kl].getDest();
			for(int i = 0; i < substrate->getN(); i++){				
				if(!location || requests[v]->getGraph()->getDist(l, i) <= requests[v]->getMaxD()){
					sprintf(cName, "constEnt_%d_%d_%d", v, kl, i);
					constraint_entrada[v][kl][i] = -z[v][l][i] <= 0;
					model.add(constraint_entrada[v][kl][i]);
					constraint_entrada[v][kl][i].setName(cName);
				}
			}
		}
	}

}

void GC::addColumns(std::vector<Column> colunas){

	for(int m=0; m < colunas.size(); m++){
		if(colunas[m].lowerBound + colunas[m].upperBound == 0)
			continue;

		int v = colunas[m].v;
		int kl = colunas[m].kl;

		char buffer[30];
		sprintf(buffer, "lambda_%d_%d_%lld", v, kl,
			static_cast<long long>(lambda.getSize()));

		IloNumVar new_variable;

		new_variable = IloNumVar(env, colunas[m].lowerBound, colunas[m].upperBound, buffer);
		lambda.add(new_variable);
		model.add(new_variable);

		objective.setLinearCoef(new_variable, colunas[m].custoFO);
		constraint_lambda[v][kl].setLinearCoef(new_variable, 1);

		std::vector<Edge> edges = colunas[m].getEdges();
		for(int e=0; e<edges.size(); e++){
			int ij = edges[e].getId();

			constraint_bw[ij].setLinearCoef(new_variable, requests[v]->getGraph()->getEdges()[kl].getBW());
		}

		if(colunas[m].k != -1 ){
			constraint_saida[v][kl][colunas[m].k].setLinearCoef(new_variable, 1);
		}
		if(colunas[m].l != -1){
			constraint_entrada[v][kl][colunas[m].l].setLinearCoef(new_variable, 1);
		}

		this->pool.push_back(colunas[m]);

		nCols++;
	}

}

void GC::getDuals(IloNumArray2 * gamma, IloNumArray3 * alpha, IloNumArray3 * pi, IloNumArray * beta){

	for (int v = 0; v < requests.size(); v++) {
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			(*gamma)[v][kl] = master->getDual(constraint_lambda[v][kl]);
		}
	}
	
	// Bandwidth constraints
	for (int i = 0; i < substrate->getM(); i++) {
		for (int j = i; j < substrate->getN(); j++) {
			if(substrate->getAdj(i,j) != -1){
				(*beta)[substrate->getAdj(i,j)] = master->getDual(constraint_bw[substrate->getAdj(i,j)]);
			}
		}
	}

	for (int v = 0; v < requests.size(); v++) {
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			int k = requests[v]->getGraph()->getEdges()[kl].getOrig();
			int l = requests[v]->getGraph()->getEdges()[kl].getDest();
			for(int i = 0; i < substrate->getN(); i++){				
				if(!location || requests[v]->getGraph()->getDist(k, i) <= requests[v]->getMaxD()){
					(*alpha)[v][kl][i] = master->getDual(constraint_saida[v][kl][i]);
				}
			}
		}
	}

	for (int v = 0; v < requests.size(); v++) {
		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			int l = requests[v]->getGraph()->getEdges()[kl].getDest();
			for(int i = 0; i < substrate->getN(); i++){				
				if(!location || requests[v]->getGraph()->getDist(l, i) <= requests[v]->getMaxD()){
					(*pi)[v][kl][i] = master->getDual(constraint_entrada[v][kl][i]);
				}
			}
		}
	}

}


void GC::addCpuCoverCutToModel(const CpuCoverCut& cut) {
	IloExpr expression(env);
	for (const std::pair<int, int>& virtualNode : cut.virtualNodes) {
		expression += z[virtualNode.first][virtualNode.second][cut.physicalNode];
	}

	IloRange range = expression <= static_cast<int>(cut.virtualNodes.size()) - 1;
	constraint_cpu_cover.add(range);
	model.add(range);
	expression.end();
}


bool GC::hasCpuCoverCut(const CpuCoverCut& cut) const {
	for (const CpuCoverCut& existing : cpuCoverCuts) {
		if (existing.physicalNode == cut.physicalNode &&
			existing.virtualNodes == cut.virtualNodes) {
			return true;
		}
	}
	return false;
}


unsigned int GC::separateCpuCoverCuts() {
	struct Candidate {
		int request;
		int virtualNode;
		double demand;
		double value;
	};

	constexpr double tolerance = 1e-6;
	std::vector<CpuCoverCut> pendingCuts;

	for (int physicalNode = 0; physicalNode < substrate->getN(); ++physicalNode) {
		std::vector<Candidate> candidates;
		for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
			for (int k = 0; k < requests[v]->getGraph()->getN(); ++k) {
				if (location && requests[v]->getGraph()->getDist(k, physicalNode) >
					requests[v]->getMaxD()) {
					continue;
				}

				const double value = master->getValue(z[v][k][physicalNode]);
				if (value <= tolerance) {
					continue;
				}

				candidates.push_back({v, k,
					requests[v]->getGraph()->getNodes()[k].getCPU(), value});
			}
		}

		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate& left, const Candidate& right) {
				if (left.value != right.value) {
					return left.value > right.value;
				}
				return left.demand > right.demand;
			});

		const double capacity = substrate->getNodes()[physicalNode].getCPU();
		std::vector<Candidate> cover;
		double coverDemand = 0.0;
		for (const Candidate& candidate : candidates) {
			cover.push_back(candidate);
			coverDemand += candidate.demand;
			if (coverDemand > capacity + tolerance) {
				break;
			}
		}

		if (coverDemand <= capacity + tolerance) {
			continue;
		}

		bool reduced = true;
		while (reduced && cover.size() > 1) {
			reduced = false;
			for (std::size_t index = 0; index < cover.size(); ++index) {
				if (coverDemand - cover[index].demand > capacity + tolerance) {
					coverDemand -= cover[index].demand;
					cover.erase(cover.begin() + index);
					reduced = true;
					break;
				}
			}
		}

		double leftHandSide = 0.0;
		CpuCoverCut cut;
		cut.physicalNode = physicalNode;
		for (const Candidate& candidate : cover) {
			leftHandSide += candidate.value;
			cut.virtualNodes.emplace_back(candidate.request, candidate.virtualNode);
		}
		std::sort(cut.virtualNodes.begin(), cut.virtualNodes.end());

		const double rightHandSide = static_cast<double>(cut.virtualNodes.size()) - 1.0;
		if (leftHandSide <= rightHandSide + tolerance || hasCpuCoverCut(cut)) {
			continue;
		}

		pendingCuts.push_back(cut);
	}

	for (const CpuCoverCut& cut : pendingCuts) {
		cpuCoverCuts.push_back(cut);
		addCpuCoverCutToModel(cpuCoverCuts.back());
	}

	return static_cast<unsigned int>(pendingCuts.size());
}


double GC::getGAP(){
	return 100*(1 - lb/ub);
}

void GC::Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, bool useCuts, int *y_, Branch *branch, unsigned int *saida){

	this->substrate = substrate;
	this->requests = requests;

	this->location = location;

	env = IloEnv();
	model = IloModel(env);
	model.setName("Master Problem - Column Generation - Path Generation");

	master = new IloCplex(model);

	IloNumArray3 alpha, pi;
	IloNumArray2 gamma;
	IloNumArray beta;
	
	alpha = IloNumArray3(env, requests.size());
	pi = IloNumArray3(env, requests.size());
	gamma = IloNumArray2(env, requests.size());

	for(int v=0; v<requests.size(); v++){
		alpha[v] = IloNumArray2(env, requests[v]->getGraph()->getM());
		pi[v] = IloNumArray2(env, requests[v]->getGraph()->getM());

		gamma[v] = IloNumArray(env, requests[v]->getGraph()->getM());

		for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
			alpha[v][kl] = IloNumArray(env, substrate->getN());
			pi[v][kl] = IloNumArray(env, substrate->getN());
		}
	}

	beta = IloNumArray(env, substrate->getM());

	CreateVariables();
	CreateObjectiveFunction();
	CreateConstraints();

	SetCplexParameters();

	Pricing * p = new Pricing();
	tempoSub = tempoMaster = tempoCuts = 0.0;
	nCuts = static_cast<unsigned int>(cpuCoverCuts.size());
	gCuts = 0;
	double init, end;

	if(this->id == 1){
		for(int v=0; v<requests.size(); v++){
			for(int kl=0; kl<requests[v]->getGraph()->getM(); kl++){
				Column c(v, kl);
				c.custoFO = M + 1;
				parentPool.push_back(c);
			}
		}
	}


	cout << "Branching decisions:" << endl;
	for(int b=0; b<branchs.size(); b++){

		if(branchs[b].tipo_branch == 3){
			cout << "y " << branchs[b].v << " = " << branchs[b].valor << endl;
		} else if(branchs[b].tipo_branch == 2){
			cout << "x " << branchs[b].v << " : " << branchs[b].x << " : " << branchs[b].y << " = " << branchs[b].valor << endl;
		} else if(branchs[b].tipo_branch == 1){
			cout << "z " << branchs[b].v << " : " << branchs[b].x << " : " << branchs[b].y << " = " << branchs[b].valor << endl;
		}

		if(branchs[b].tipo_branch == 3){
			y[branchs[b].v].setBounds(branchs[b].valor, branchs[b].valor);
		} else if(branchs[b].tipo_branch == 1){
			int v = branchs[b].v;
			int k = branchs[b].x;
			int i = branchs[b].y;
			int valor = branchs[b].valor;

			z[v][k][i].setBounds(valor, valor);
		} 
	}

	std::vector<Column> colunas = std::vector<Column>();
	addColumns(parentPool);
	gCols = nCols;

	for(int b=0; b<branchs.size(); b++){
		if(branchs[b].tipo_branch == 3){
			y[branchs[b].v].setBounds(branchs[b].valor, branchs[b].valor);
		}
	}


	while(true){
		init = get_time();
		if(!master->solve()){
			this->lb = VNE_INFINITY;
			delete p;
			delete master;
			env.end();
			return;
		}
		end =  get_time();
		tempoMaster += end - init;

		if (useCuts) {
			init = get_time();
			const unsigned int addedCuts = separateCpuCoverCuts();
			end = get_time();
			tempoCuts += end - init;
			gCuts += addedCuts;
			nCuts = static_cast<unsigned int>(cpuCoverCuts.size());
			if (addedCuts > 0) {
				continue;
			}
		}

		//cout << " ~ " << master->getObjValue() << endl;

		getDuals(&gamma, &alpha, &pi, &beta);

		init = get_time();
		p->Solve(substrate, requests, location, delay, resilience, gamma, alpha, pi, beta, &colunas, forbidden, branchs);
		end =  get_time();
		tempoSub += end - init;

			// Resolver Pricing
		if(colunas.size() == 0)
			break;

		addColumns(colunas);
		colunas.clear();
	}
	gCols = nCols - gCols;

	tempoRelaxacao = tempoMaster + tempoSub + tempoCuts;
	tempoTotal = tempoRelaxacao;

	this->lb = master->getObjValue();
	*saida = 0;

	for (int m = 0; m < lambda.getSize(); m++) {
		double value = master->getValue(lambda[m]);
		if(value >= 0.001 && pool[m].custoFO >= 10000){
			this->lb = VNE_INFINITY;
			delete p;
			delete master;
			env.end();
			return;
		}
	}

	// Existe y fracionário?
	double mais_frac = 0.001;
	for (int v = 0; v < requests.size(); v++){
		double value = master->getValue(y[v]);
		if(abs(value - std::round(value)) > mais_frac){
			mais_frac = abs(value - std::round(value));
			sol_inteira = false;
			*saida = 1;
			*y_ = v;
			branch->v = v;
			branch->tipo_branch = 3;
		}
	}

	if(*saida == 0){
		mais_frac = 0.001;
		for (int v = 0; v < requests.size(); v++) {
			for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
				for (int i = 0; i < substrate->getN(); i++) {
					if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
						continue;
					double value = master->getValue(z[v][k][i]);
					if(abs(value - std::round(value)) > mais_frac){
						mais_frac = abs(value - std::round(value));
						sol_inteira = false;
						*saida = 1;

						branch->v = v;
						branch->x = k;
						branch->y = i;
						branch->tipo_branch = 1;
					}
				}
			}
		}
	}

	// Existe lambda fracionário?
	if(*saida == 0){
		mais_frac = 0.001;

		double *** uso = new double**[requests.size()];
		for (int v = 0; v < requests.size(); v++) {
			uso[v] = new double*[requests[v]->getGraph()->getM()];
			for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
				uso[v][kl] = new double[substrate->getM()];
				for(int ij=0; ij<substrate->getM(); ij++){
					uso[v][kl][ij] = 0.0;
				}
			}
		}

		for (int m = 0; m < lambda.getSize(); m++) {
			double value = master->getValue(lambda[m]);
			value = abs(value - std::round(value));
			
			double valorLambda = master->getValue(lambda[m]);
			
			if(value >= 0.001)
				sol_inteira = false;

			int v = pool[m].v;
			int kl = pool[m].kl;

			std::vector<Edge> edges = pool[m].getEdges();
			for(int e=0; e<edges.size(); e++){
				int ij = edges[e].getId();
				
				uso[v][kl][ij] += valorLambda;

				
			}
		}

		double maior_ = 0.0;
		for (int v = 0; v < requests.size(); v++) {
			for (int kl = 0; kl < requests[v]->getGraph()->getM(); kl++) {
				for(int ij=0; ij<substrate->getM(); ij++){

					if(abs(uso[v][kl][ij] - std::round(uso[v][kl][ij])) > maior_){
						maior_ = abs(uso[v][kl][ij] - std::round(uso[v][kl][ij]));

						branch->v = v;
						branch->x = kl;
						branch->y = ij;
						branch->tipo_branch = 2;

						*saida = 1;
					}
				}
			}
		}

	}

	if(sol_inteira){
		this->ub = this->lb;
	 } else {

	 	std::vector<int> lista = std::vector<int>();

	 	for(int p_=0; p_<pool.size(); p_++){
	 		if(master->getValue(lambda[p_]) <= 0){
	 			lista.push_back(p_);
	 		}
	 	}

	 	for (int v = 0; v < requests.size(); v++) {
	 		for (int k = 0; k < requests[v]->getGraph()->getN(); k++) {
	 			for (int i = 0; i < substrate->getN(); i++) {
	 				if (location && requests[v]->getGraph()->getDist(k, i) > requests[v]->getMaxD())
	 					continue;
	 				model.add(IloConversion(env, z[v][k][i], ILOBOOL));
	 			}
	 		}
	 	}
	 	model.add(IloConversion(env, lambda, ILOBOOL));
	 	model.add(IloConversion(env, y, ILOBOOL));

	 	master->setParam(IloCplex::TiLim, 30.0);
	 	//master->setParam(IloCplex::IntSolLim, 1);

		for(int l_=0; l_<lista.size(); l_++){
			int p_ = lista[l_];
			//lambda[p_].setBounds(0, 0);
		}

	 	try{
		init = get_time();
		if(master->solve())
			this->ub = master->getObjValue();
		else
			this->ub = VNE_INFINITY;
		end = get_time();
		tempoTotal += end - init;
		//cout << "Custo Inteiro: " << this->ub << endl;
		} catch (IloException e){
			cout << " O bizil foi aqui!" << endl;
		}
	}

	delete p;
	delete master;
	env.end();
	return;
}
