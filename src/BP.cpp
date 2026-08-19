#include "BP.h"
#include "Branch.h"
#include "Heuristica.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

namespace {
constexpr char TREE_SEPARATOR = ' ';
constexpr int TREE_COLUMN_WIDTH = 15;

class WorkerPool {
public:
	explicit WorkerPool(unsigned int threadCount) {
		for (unsigned int index = 0; index < threadCount; ++index) {
			workers.emplace_back([this]() {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(mutex);
						condition.wait(lock, [this]() { return stopping || !tasks.empty(); });
						if (stopping && tasks.empty()) return;
						task = std::move(tasks.front());
						tasks.pop();
					}
					task();
				}
			});
		}
	}

	~WorkerPool() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopping = true;
		}
		condition.notify_all();
		for (std::thread& worker : workers) {
			worker.join();
		}
	}

	WorkerPool(const WorkerPool&) = delete;
	WorkerPool& operator=(const WorkerPool&) = delete;

	template <typename Function>
	auto submit(Function&& function)
		-> std::future<std::invoke_result_t<Function>> {
		using Result = std::invoke_result_t<Function>;
		auto task = std::make_shared<std::packaged_task<Result()>>(
			std::forward<Function>(function));
		std::future<Result> future = task->get_future();
		{
			std::lock_guard<std::mutex> lock(mutex);
			tasks.emplace([task]() { (*task)(); });
		}
		condition.notify_one();
		return future;
	}

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::mutex mutex;
	std::condition_variable condition;
	bool stopping = false;
};

struct NodePriority {
	bool operator()(const GC* left, const GC* right) const {
		if (left->parentLB != right->parentLB) {
			return left->parentLB > right->parentLB;
		}
		return left->id > right->id;
	}
};

struct NodeSolveResult {
	GC* node = nullptr;
	Branch branch;
	unsigned int branchRequired = 0;
};

struct RunningNode {
	GC* node = nullptr;
	std::future<NodeSolveResult> future;
};

void writeTreeHeader(std::ostream& output) {
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "ID";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Open Nodes";
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << "Active";
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
	double globalLowerBound, size_t openNodes, size_t activeWorkers) {
	const double gap = bestUpperBound > 0.0 ?
		100.0 * (1.0 - globalLowerBound / bestUpperBound) : 0.0;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << node.id;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << openNodes;
	output << left << setw(TREE_COLUMN_WIDTH) << setfill(TREE_SEPARATOR) << activeWorkers;
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

	std::priority_queue<GC*, std::vector<GC*>, NodePriority> nodePool;
	GC * raiz = new GC();
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

	nodePool.push(raiz);

    double worstLB;
    double tempoExecucao = 0;
	const double globalStart = get_time();
	unsigned int processedNodes = 0;
	unsigned int totalCgIterations = 0;
	unsigned int totalGeneratedColumns = 0;
	unsigned int totalDuplicateColumns = 0;
	unsigned int maxActiveWorkers = 0;
	double rootLowerBound = 0.0;
	double interruptedLowerBound = VNE_INFINITY;
	std::string terminationReason = "tree_exhausted";
	bool stopLaunching = false;
	const unsigned int effectiveTreeThreads = config.rootOnly ? 1U : config.treeThreads;
	WorkerPool workerPool(effectiveTreeThreads);
	std::vector<RunningNode> runningNodes;

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

	auto calculateGlobalLowerBound = [&]() {
		double lowerBound = bestUB;
		if (!nodePool.empty()) {
			lowerBound = std::min(lowerBound, nodePool.top()->parentLB);
		}
		for (const RunningNode& running : runningNodes) {
			lowerBound = std::min(lowerBound, running.node->parentLB);
		}
		lowerBound = std::min(lowerBound, interruptedLowerBound);
		return lowerBound;
	};

	while (!nodePool.empty() || !runningNodes.empty()) {
		tempoExecucao = get_time() - globalStart;
		if (tempoExecucao >= config.globalTimeLimitSeconds) {
			stopLaunching = true;
			terminationReason = "global_time_limit";
		}

		while (!stopLaunching && runningNodes.size() < effectiveTreeThreads &&
			!nodePool.empty()) {
			GC* node = nodePool.top();
			nodePool.pop();
			if (node->parentLB >= bestUB) {
				delete node;
				continue;
			}

			const double remainingGlobal = std::max(0.0,
				config.globalTimeLimitSeconds - (get_time() - globalStart));
			if (remainingGlobal <= 0.01) {
				nodePool.push(node);
				stopLaunching = true;
				terminationReason = "global_time_limit";
				break;
			}

			auto future = workerPool.submit([node, substrate, &requests, location,
				delay, resilience, useCuts, &config, remainingGlobal]() {
				NodeSolveResult result;
				result.node = node;
				int acceptanceVariable = 0;
				node->Solve(substrate, requests, location, delay, resilience,
					useCuts, &acceptanceVariable, &result.branch,
					&result.branchRequired, config, remainingGlobal);
				return result;
			});
			runningNodes.push_back({node, std::move(future)});
			maxActiveWorkers = std::max(maxActiveWorkers,
				static_cast<unsigned int>(runningNodes.size()));
		}

		if (runningNodes.empty()) {
			break;
		}

		size_t readyIndex = runningNodes.size();
		for (size_t index = 0; index < runningNodes.size(); ++index) {
			if (runningNodes[index].future.wait_for(std::chrono::milliseconds(0)) ==
				std::future_status::ready) {
				readyIndex = index;
				break;
			}
		}
		if (readyIndex == runningNodes.size()) {
			runningNodes.front().future.wait_for(std::chrono::milliseconds(10));
			continue;
		}

		NodeSolveResult result;
		GC* completedNode = runningNodes[readyIndex].node;
		try {
			result = runningNodes[readyIndex].future.get();
		} catch (const std::exception& error) {
			cerr << "Falha ao resolver no " << completedNode->id << ": "
				<< error.what() << endl;
			interruptedLowerBound = std::min(interruptedLowerBound,
				completedNode->parentLB);
			terminationReason = "worker_exception";
			stopLaunching = true;
			delete completedNode;
			runningNodes.erase(runningNodes.begin() + readyIndex);
			continue;
		}
		runningNodes.erase(runningNodes.begin() + readyIndex);
		GC* node = result.node;

		processedNodes++;
		totalCgIterations += node->cgIterations;
		totalGeneratedColumns += node->gCols;
		totalDuplicateColumns += node->duplicateColumns;
		if (node->id == 1) rootLowerBound = node->lb;
		if (node->ub < bestUB) bestUB = node->ub;

		if (node->relaxationComplete && !config.rootOnly &&
			result.branchRequired == 1 && node->lb < bestUB) {
			for (int value = 0; value <= 1; ++value) {
				GC* child = new GC(node);
				child->addBranch(result.branch, value);
				child->id = node->id * 2 + value;
				nodePool.push(child);
			}
		}

		if (!node->relaxationComplete) {
			interruptedLowerBound = std::min(interruptedLowerBound, node->lb);
			if (terminationReason != "global_time_limit") {
				terminationReason = node->cgStopReason;
			}
			stopLaunching = true;
		}
		if (config.rootOnly && node->relaxationComplete) {
			terminationReason = "root_only";
			stopLaunching = true;
		}

		worstLB = calculateGlobalLowerBound();
		writeTreeRow(ofs, *node, bestUB, worstLB, nodePool.size(),
			runningNodes.size());
		writeTreeRow(cout, *node, bestUB, worstLB, nodePool.size(),
			runningNodes.size());
		delete node;
	}

	worstLB = bestUB;
	if (config.rootOnly) {
		worstLB = rootLowerBound;
	} else {
		if (!nodePool.empty()) {
			worstLB = std::min(worstLB, nodePool.top()->parentLB);
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
	ofs << "tree_threads=" << effectiveTreeThreads << endl;
	ofs << "max_active_workers=" << maxActiveWorkers << endl;
	ofs << "open_nodes_remaining=" << nodePool.size() << endl;
	ofs << "branching=most_fractional" << endl;
	ofs << "END_SUMMARY" << endl;
	ofs << "FINISHED!" << endl;

	ofs.close();
	delete [] redeAceita;
	while (!nodePool.empty()) {
		delete nodePool.top();
		nodePool.pop();
	}
}
