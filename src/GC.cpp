#include "GC.h"
#include "FeasibilityOracle.h"

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
	nCpuCuts = gCpuCuts = 0;
	nBandwidthCuts = gBandwidthCuts = 0;
	nAcceptanceCuts = gAcceptanceCuts = 0;
	nNoGoodCuts = gNoGoodCuts = 0;
	nFeasibilityChecks = nFeasibilityUnknown = 0;
	nextColumnId = 0;

	pool = std::vector<Column>();
	parentPool = std::vector<Column>();
	forbidden = std::vector<Column>();
	branchs = std::vector<Branch>();
	cpuCoverCuts = std::vector<CpuCoverCut>();
	bandwidthCoverCuts = std::vector<BandwidthCoverCut>();
	acceptanceResourceCoverCuts = std::vector<AcceptanceResourceCoverCut>();
	acceptanceResourceProfiles = std::vector<AcceptanceResourceProfile>();
	acceptanceNoGoodCuts = std::vector<AcceptanceNoGoodCut>();
	feasibleAcceptanceSets = std::vector<std::vector<int>>();
	inconclusiveAcceptanceSets = std::vector<std::vector<int>>();
}

GC::GC(GC * parent){
	id = 0;
	lb = parent->lb;
	ub = VNE_INFINITY;
	sol_inteira = true;
	nCols = 0;
	gCols = 0;
	nCuts = static_cast<unsigned int>(parent->cpuCoverCuts.size() +
		parent->bandwidthCoverCuts.size() + parent->acceptanceResourceCoverCuts.size() +
		parent->acceptanceNoGoodCuts.size());
	gCuts = 0;
	nCpuCuts = static_cast<unsigned int>(parent->cpuCoverCuts.size());
	nBandwidthCuts = static_cast<unsigned int>(parent->bandwidthCoverCuts.size());
	nAcceptanceCuts = static_cast<unsigned int>(parent->acceptanceResourceCoverCuts.size());
	nNoGoodCuts = static_cast<unsigned int>(parent->acceptanceNoGoodCuts.size());
	gCpuCuts = gBandwidthCuts = gAcceptanceCuts = gNoGoodCuts = 0;
	nFeasibilityChecks = nFeasibilityUnknown = 0;
	nextColumnId = parent->nextColumnId;
	this->pool = std::vector<Column>();
	this->parentPool = std::vector<Column>(parent->pool);
	this->branchs = std::vector<Branch>(parent->branchs);
	this->forbidden = std::vector<Column>(parent->forbidden);
	this->cpuCoverCuts = std::vector<CpuCoverCut>(parent->cpuCoverCuts);
	this->bandwidthCoverCuts = std::vector<BandwidthCoverCut>(parent->bandwidthCoverCuts);
	this->acceptanceResourceCoverCuts =
		std::vector<AcceptanceResourceCoverCut>(parent->acceptanceResourceCoverCuts);
	this->acceptanceResourceProfiles = std::vector<AcceptanceResourceProfile>();
	this->acceptanceNoGoodCuts =
		std::vector<AcceptanceNoGoodCut>(parent->acceptanceNoGoodCuts);
	this->feasibleAcceptanceSets =
		std::vector<std::vector<int>>(parent->feasibleAcceptanceSets);
	this->inconclusiveAcceptanceSets =
		std::vector<std::vector<int>>(parent->inconclusiveAcceptanceSets);
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
	constraint_bandwidth_cover = OneDimRange(env);
	constraint_acceptance_resource_cover = OneDimRange(env);
	constraint_acceptance_nogood = OneDimRange(env);

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
	for (const AcceptanceResourceCoverCut& cut : acceptanceResourceCoverCuts) {
		addAcceptanceResourceCoverCutToModel(cut);
	}
	for (const AcceptanceNoGoodCut& cut : acceptanceNoGoodCuts) {
		addAcceptanceNoGoodCutToModel(cut);
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
	for (int i = 0; i < substrate->getN(); i++) {
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

		if (colunas[m].id < 0) {
			colunas[m].id = nextColumnId++;
		} else if (colunas[m].id >= nextColumnId) {
			nextColumnId = colunas[m].id + 1;
		}

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


void GC::addBandwidthCoverCutToModel(const BandwidthCoverCut& cut) {
	IloExpr expression(env);
	for (long long columnId : cut.columnIds) {
		for (int index = 0; index < static_cast<int>(pool.size()); ++index) {
			if (pool[index].id == columnId) {
				expression += lambda[index];
				break;
			}
		}
	}

	IloRange range = expression <= static_cast<int>(cut.columnIds.size()) - 1;
	constraint_bandwidth_cover.add(range);
	model.add(range);
	expression.end();
}


bool GC::hasBandwidthCoverCut(const BandwidthCoverCut& cut) const {
	for (const BandwidthCoverCut& existing : bandwidthCoverCuts) {
		if (existing.physicalEdge == cut.physicalEdge &&
			existing.columnIds == cut.columnIds) {
			return true;
		}
	}
	return false;
}


void GC::addAcceptanceResourceCoverCutToModel(
	const AcceptanceResourceCoverCut& cut) {
	IloExpr expression(env);
	for (int request : cut.requests) {
		expression += y[request];
	}

	IloRange range = expression <= static_cast<int>(cut.requests.size()) - 1;
	constraint_acceptance_resource_cover.add(range);
	model.add(range);
	expression.end();
}


bool GC::hasAcceptanceResourceCoverCut(
	const AcceptanceResourceCoverCut& cut) const {
	for (const AcceptanceResourceCoverCut& existing : acceptanceResourceCoverCuts) {
		// The inequality is fully identified by its request set. Different resource
		// profiles can prove the same cover, but it only needs to be added once.
		if (existing.requests == cut.requests) {
			return true;
		}
	}
	return false;
}


void GC::addAcceptanceNoGoodCutToModel(const AcceptanceNoGoodCut& cut) {
	IloExpr expression(env);
	for (int request : cut.requests) {
		expression += y[request];
	}
	IloRange range = expression <= static_cast<int>(cut.requests.size()) - 1;
	constraint_acceptance_nogood.add(range);
	model.add(range);
	expression.end();
}


bool GC::hasAcceptanceNoGoodCut(const AcceptanceNoGoodCut& cut) const {
	for (const AcceptanceNoGoodCut& existing : acceptanceNoGoodCuts) {
		if (existing.requests == cut.requests) {
			return true;
		}
	}
	return false;
}


void GC::buildAcceptanceResourceProfiles() {
	constexpr int totalCpu = 0;
	constexpr int cpuThreshold = 1;
	constexpr int totalBandwidth = 2;
	constexpr int bandwidthThreshold = 3;
	constexpr double tolerance = 1e-9;
	constexpr std::size_t maxThresholds = 64;

	acceptanceResourceProfiles.clear();

	auto addProfileIfUseful = [this, tolerance](int resourceKind, double threshold,
		const std::vector<double>& weights, double capacity) {
		double totalWeight = 0.0;
		for (double weight : weights) {
			totalWeight += weight;
		}
		if (totalWeight > capacity + tolerance) {
			acceptanceResourceProfiles.push_back(
				{resourceKind, threshold, weights, capacity});
		}
	};

	std::vector<double> cpuWeights(requests.size(), 0.0);
	std::vector<double> bandwidthWeights(requests.size(), 0.0);
	std::vector<double> cpuThresholds;
	std::vector<double> bandwidthThresholds;
	double totalCpuCapacity = 0.0;
	double totalBandwidthCapacity = 0.0;

	for (const Node& node : substrate->getNodes()) {
		totalCpuCapacity += node.getCPU();
	}
	for (const Edge& edge : substrate->getEdges()) {
		totalBandwidthCapacity += edge.getBW();
	}

	for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
		for (const Node& node : requests[v]->getGraph()->getNodes()) {
			cpuWeights[v] += node.getCPU();
			if (node.getCPU() > tolerance) {
				cpuThresholds.push_back(node.getCPU());
			}
		}
		for (const Edge& edge : requests[v]->getGraph()->getEdges()) {
			bandwidthWeights[v] += edge.getBW();
			if (edge.getBW() > tolerance) {
				bandwidthThresholds.push_back(edge.getBW());
			}
		}
	}

	addProfileIfUseful(totalCpu, 0.0, cpuWeights, totalCpuCapacity);
	addProfileIfUseful(totalBandwidth, 0.0, bandwidthWeights,
		totalBandwidthCapacity);

	auto prepareThresholds = [maxThresholds](std::vector<double> thresholds) {
		std::sort(thresholds.begin(), thresholds.end());
		thresholds.erase(std::unique(thresholds.begin(), thresholds.end()),
			thresholds.end());
		if (thresholds.size() <= maxThresholds) {
			return thresholds;
		}

		std::vector<double> sampled;
		sampled.reserve(maxThresholds);
		for (std::size_t index = 0; index < maxThresholds; ++index) {
			const std::size_t source = index * (thresholds.size() - 1) /
				(maxThresholds - 1);
			if (sampled.empty() || sampled.back() != thresholds[source]) {
				sampled.push_back(thresholds[source]);
			}
		}
		return sampled;
	};

	for (double threshold : prepareThresholds(cpuThresholds)) {
		std::vector<double> weights(requests.size(), 0.0);
		double capacity = 0.0;
		for (const Node& node : substrate->getNodes()) {
			capacity += std::floor(node.getCPU() / threshold + tolerance);
		}
		for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
			for (const Node& node : requests[v]->getGraph()->getNodes()) {
				if (node.getCPU() + tolerance >= threshold) {
					weights[v] += 1.0;
				}
			}
		}
		addProfileIfUseful(cpuThreshold, threshold, weights, capacity);
	}

	for (double threshold : prepareThresholds(bandwidthThresholds)) {
		std::vector<double> weights(requests.size(), 0.0);
		double capacity = 0.0;
		for (const Edge& edge : substrate->getEdges()) {
			capacity += std::floor(edge.getBW() / threshold + tolerance);
		}
		for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
			for (const Edge& edge : requests[v]->getGraph()->getEdges()) {
				if (edge.getBW() + tolerance >= threshold) {
					weights[v] += 1.0;
				}
			}
		}
		addProfileIfUseful(bandwidthThreshold, threshold, weights, capacity);
	}
}


std::vector<CpuCoverCut> GC::findViolatedCpuCoverCuts() {
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

	return pendingCuts;
}


std::vector<BandwidthCoverCut> GC::findViolatedBandwidthCoverCuts() {
	struct Candidate {
		long long columnId;
		double demand;
		double value;
	};

	constexpr double tolerance = 1e-6;
	std::vector<BandwidthCoverCut> pendingCuts;

	for (int physicalEdge = 0; physicalEdge < substrate->getM(); ++physicalEdge) {
		std::vector<Candidate> candidates;
		for (int index = 0; index < static_cast<int>(pool.size()); ++index) {
			const double value = master->getValue(lambda[index]);
			if (value <= tolerance) {
				continue;
			}

			bool usesEdge = false;
			for (const Edge& edge : pool[index].getEdges()) {
				if (edge.getId() == physicalEdge) {
					usesEdge = true;
					break;
				}
			}
			if (!usesEdge) {
				continue;
			}

			const int request = pool[index].v;
			const int virtualEdge = pool[index].kl;
			candidates.push_back({pool[index].id,
				requests[request]->getGraph()->getEdges()[virtualEdge].getBW(), value});
		}

		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate& left, const Candidate& right) {
				if (left.value != right.value) {
					return left.value > right.value;
				}
				return left.demand > right.demand;
			});

		const double capacity = substrate->getEdges()[physicalEdge].getBW();
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
		BandwidthCoverCut cut;
		cut.physicalEdge = physicalEdge;
		for (const Candidate& candidate : cover) {
			leftHandSide += candidate.value;
			cut.columnIds.push_back(candidate.columnId);
		}
		std::sort(cut.columnIds.begin(), cut.columnIds.end());

		const double rightHandSide = static_cast<double>(cut.columnIds.size()) - 1.0;
		if (leftHandSide <= rightHandSide + tolerance || hasBandwidthCoverCut(cut)) {
			continue;
		}

		pendingCuts.push_back(cut);
	}

	return pendingCuts;
}


std::vector<AcceptanceResourceCoverCut>
GC::findViolatedAcceptanceResourceCoverCuts() {
	struct Candidate {
		int request;
		double weight;
		double value;
	};

	constexpr double tolerance = 1e-6;
	std::vector<AcceptanceResourceCoverCut> pendingCuts;

	for (const AcceptanceResourceProfile& profile : acceptanceResourceProfiles) {
		std::vector<Candidate> candidates;
		for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
			const double value = master->getValue(y[v]);
			if (value > tolerance && profile.weights[v] > tolerance) {
				candidates.push_back({v, profile.weights[v], value});
			}
		}

		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate& left, const Candidate& right) {
				if (left.value != right.value) {
					return left.value > right.value;
				}
				return left.weight > right.weight;
			});

		std::vector<Candidate> cover;
		double coverWeight = 0.0;
		for (const Candidate& candidate : candidates) {
			cover.push_back(candidate);
			coverWeight += candidate.weight;
			if (coverWeight > profile.capacity + tolerance) {
				break;
			}
		}
		if (coverWeight <= profile.capacity + tolerance) {
			continue;
		}

		bool reduced = true;
		while (reduced && cover.size() > 1) {
			reduced = false;
			for (std::size_t index = 0; index < cover.size(); ++index) {
				if (coverWeight - cover[index].weight >
					profile.capacity + tolerance) {
					coverWeight -= cover[index].weight;
					cover.erase(cover.begin() + index);
					reduced = true;
					break;
				}
			}
		}

		double leftHandSide = 0.0;
		AcceptanceResourceCoverCut cut;
		cut.resourceKind = profile.resourceKind;
		cut.threshold = profile.threshold;
		for (const Candidate& candidate : cover) {
			leftHandSide += candidate.value;
			cut.requests.push_back(candidate.request);
		}
		std::sort(cut.requests.begin(), cut.requests.end());

		const double rightHandSide = static_cast<double>(cut.requests.size()) - 1.0;
		if (leftHandSide <= rightHandSide + tolerance ||
			hasAcceptanceResourceCoverCut(cut)) {
			continue;
		}

		bool alreadyPending = false;
		for (const AcceptanceResourceCoverCut& pending : pendingCuts) {
			if (pending.requests == cut.requests) {
				alreadyPending = true;
				break;
			}
		}
		if (!alreadyPending) {
			pendingCuts.push_back(cut);
		}
	}

	return pendingCuts;
}


std::vector<AcceptanceNoGoodCut> GC::findViolatedAcceptanceNoGoodCuts() {
	constexpr double tolerance = 1e-6;
	constexpr long long oracleVariableBudget = 850;
	constexpr double oracleTimeLimitSeconds = 5.0;

	struct Candidate {
		int request;
		double value;
		long long variableEstimate;
	};

	std::vector<Candidate> candidates;
	for (int v = 0; v < static_cast<int>(requests.size()); ++v) {
		const double value = master->getValue(y[v]);
		if (value <= tolerance) {
			continue;
		}
		const long long estimate =
			static_cast<long long>(requests[v]->getGraph()->getN()) * substrate->getN() +
			static_cast<long long>(requests[v]->getGraph()->getM()) * substrate->getM();
		candidates.push_back({v, value, estimate});
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const Candidate& left, const Candidate& right) {
			return left.value > right.value;
		});

	std::vector<int> selected;
	long long estimatedVariables = 0;
	double totalDeficit = 0.0;
	for (const Candidate& candidate : candidates) {
		const double candidateDeficit = 1.0 - candidate.value;
		if (estimatedVariables + candidate.variableEstimate > oracleVariableBudget) {
			continue;
		}
		if (totalDeficit + candidateDeficit >= 1.0 - tolerance) {
			break;
		}
		selected.push_back(candidate.request);
		estimatedVariables += candidate.variableEstimate;
		totalDeficit += candidateDeficit;
	}

	if (selected.empty()) {
		return {};
	}
	std::sort(selected.begin(), selected.end());

	auto containsSet = [](const std::vector<int>& superset,
		const std::vector<int>& subset) {
		return std::includes(superset.begin(), superset.end(),
			subset.begin(), subset.end());
	};

	for (const std::vector<int>& feasible : feasibleAcceptanceSets) {
		if (containsSet(feasible, selected)) {
			return {};
		}
	}
	for (const std::vector<int>& inconclusive : inconclusiveAcceptanceSets) {
		if (inconclusive == selected) {
			return {};
		}
	}

	auto check = [this, oracleTimeLimitSeconds](const std::vector<int>& requestSet) {
		++nFeasibilityChecks;
		const FeasibilityStatus status = FeasibilityOracle::Check(substrate, requests,
			requestSet, location, oracleTimeLimitSeconds);
		if (status == FeasibilityStatus::Unknown) {
			++nFeasibilityUnknown;
		}
		return status;
	};

	FeasibilityStatus status = check(selected);
	if (status == FeasibilityStatus::Feasible) {
		feasibleAcceptanceSets.push_back(selected);
		return {};
	}
	if (status == FeasibilityStatus::Unknown) {
		inconclusiveAcceptanceSets.push_back(selected);
		return {};
	}

	// Remove requests while infeasibility remains, producing a smaller and
	// stronger no-good cover. Unknown checks conservatively keep the request.
	for (std::size_t index = 0; index < selected.size() && selected.size() > 1;) {
		std::vector<int> trial = selected;
		trial.erase(trial.begin() + index);

		bool knownFeasible = false;
		for (const std::vector<int>& feasible : feasibleAcceptanceSets) {
			if (containsSet(feasible, trial)) {
				knownFeasible = true;
				break;
			}
		}
		if (knownFeasible) {
			++index;
			continue;
		}

		status = check(trial);
		if (status == FeasibilityStatus::Infeasible) {
			selected.swap(trial);
			continue;
		}
		if (status == FeasibilityStatus::Feasible) {
			feasibleAcceptanceSets.push_back(trial);
		} else {
			inconclusiveAcceptanceSets.push_back(trial);
		}
		++index;
	}

	AcceptanceNoGoodCut cut;
	cut.requests = selected;
	if (hasAcceptanceNoGoodCut(cut)) {
		return {};
	}
	return {cut};
}


unsigned int GC::separateCoverCuts() {
	// Both families must inspect the same LP solution. Adding any range invalidates
	// that solution in Concert, so model updates are deliberately deferred.
	const std::vector<CpuCoverCut> newCpuCuts = findViolatedCpuCoverCuts();
	const std::vector<BandwidthCoverCut> newBandwidthCuts =
		findViolatedBandwidthCoverCuts();
	const std::vector<AcceptanceResourceCoverCut> newAcceptanceCuts =
		findViolatedAcceptanceResourceCoverCuts();
	std::vector<AcceptanceNoGoodCut> newNoGoodCuts;
	if (newCpuCuts.empty() && newBandwidthCuts.empty() && newAcceptanceCuts.empty()) {
		newNoGoodCuts = findViolatedAcceptanceNoGoodCuts();
	}

	for (const CpuCoverCut& cut : newCpuCuts) {
		cpuCoverCuts.push_back(cut);
		addCpuCoverCutToModel(cpuCoverCuts.back());
	}
	for (const BandwidthCoverCut& cut : newBandwidthCuts) {
		bandwidthCoverCuts.push_back(cut);
		addBandwidthCoverCutToModel(bandwidthCoverCuts.back());
	}
	for (const AcceptanceResourceCoverCut& cut : newAcceptanceCuts) {
		acceptanceResourceCoverCuts.push_back(cut);
		addAcceptanceResourceCoverCutToModel(acceptanceResourceCoverCuts.back());
	}
	for (const AcceptanceNoGoodCut& cut : newNoGoodCuts) {
		acceptanceNoGoodCuts.push_back(cut);
		addAcceptanceNoGoodCutToModel(acceptanceNoGoodCuts.back());
	}

	gCpuCuts += static_cast<unsigned int>(newCpuCuts.size());
	gBandwidthCuts += static_cast<unsigned int>(newBandwidthCuts.size());
	gAcceptanceCuts += static_cast<unsigned int>(newAcceptanceCuts.size());
	gNoGoodCuts += static_cast<unsigned int>(newNoGoodCuts.size());
	nCpuCuts = static_cast<unsigned int>(cpuCoverCuts.size());
	nBandwidthCuts = static_cast<unsigned int>(bandwidthCoverCuts.size());
	nAcceptanceCuts = static_cast<unsigned int>(acceptanceResourceCoverCuts.size());
	nNoGoodCuts = static_cast<unsigned int>(acceptanceNoGoodCuts.size());
	nCuts = nCpuCuts + nBandwidthCuts + nAcceptanceCuts + nNoGoodCuts;
	return static_cast<unsigned int>(newCpuCuts.size() + newBandwidthCuts.size() +
		newAcceptanceCuts.size() + newNoGoodCuts.size());
}


double GC::getGAP(){
	return 100*(1 - lb/ub);
}

void GC::Solve(Graph *substrate, std::vector<Request*> requests, bool location, bool delay, bool resilience, bool useCuts, int *y_, Branch *branch, unsigned int *saida){

	this->substrate = substrate;
	this->requests = requests;

	this->location = location;
	buildAcceptanceResourceProfiles();

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
	nCpuCuts = static_cast<unsigned int>(cpuCoverCuts.size());
	nBandwidthCuts = static_cast<unsigned int>(bandwidthCoverCuts.size());
	nAcceptanceCuts = static_cast<unsigned int>(acceptanceResourceCoverCuts.size());
	nNoGoodCuts = static_cast<unsigned int>(acceptanceNoGoodCuts.size());
	nCuts = nCpuCuts + nBandwidthCuts + nAcceptanceCuts + nNoGoodCuts;
	gCuts = 0;
	gCpuCuts = gBandwidthCuts = gAcceptanceCuts = gNoGoodCuts = 0;
	nFeasibilityChecks = nFeasibilityUnknown = 0;
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
	for (const BandwidthCoverCut& cut : bandwidthCoverCuts) {
		addBandwidthCoverCutToModel(cut);
	}
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
			const unsigned int addedCuts = separateCoverCuts();
			end = get_time();
			tempoCuts += end - init;
			gCuts += addedCuts;
			nCuts = static_cast<unsigned int>(cpuCoverCuts.size() +
				bandwidthCoverCuts.size() + acceptanceResourceCoverCuts.size() +
				acceptanceNoGoodCuts.size());
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
