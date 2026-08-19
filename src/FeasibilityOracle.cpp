#include "FeasibilityOracle.h"

#include <ilcplex/ilocplex.h>

#include <chrono>
#include <vector>

typedef IloArray<IloBoolVarArray> BoolVarMatrix;
typedef IloArray<BoolVarMatrix> BoolVar3Matrix;

FeasibilityStatus FeasibilityOracle::Check(
	Graph* substrate,
	const std::vector<Request*>& requests,
	const std::vector<int>& selectedRequests,
	bool location,
	double timeLimitSeconds) {

	if (selectedRequests.empty()) {
		return FeasibilityStatus::Feasible;
	}

	IloEnv env;
	FeasibilityStatus result = FeasibilityStatus::Unknown;

	try {
		IloModel model(env);
		const int physicalNodes = substrate->getN();
		const int physicalEdges = substrate->getM();
		const int selectedCount = static_cast<int>(selectedRequests.size());

		BoolVar3Matrix placement(env, selectedCount);
		BoolVar3Matrix route(env, selectedCount);

		for (int selected = 0; selected < selectedCount; ++selected) {
			const int requestIndex = selectedRequests[selected];
			Graph* virtualGraph = requests[requestIndex]->getGraph();

			placement[selected] = BoolVarMatrix(env, virtualGraph->getN());
			for (int virtualNode = 0; virtualNode < virtualGraph->getN(); ++virtualNode) {
				placement[selected][virtualNode] = IloBoolVarArray(env, physicalNodes);
				for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
					if (location && virtualGraph->getDist(virtualNode, physicalNode) >
						requests[requestIndex]->getMaxD()) {
						continue;
					}
					placement[selected][virtualNode][physicalNode] = IloBoolVar(env);
					model.add(placement[selected][virtualNode][physicalNode]);
				}
			}

			route[selected] = BoolVarMatrix(env, virtualGraph->getM());
			for (int virtualEdge = 0; virtualEdge < virtualGraph->getM(); ++virtualEdge) {
				route[selected][virtualEdge] = IloBoolVarArray(env, physicalEdges);
				for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
					route[selected][virtualEdge][physicalEdge] = IloBoolVar(env);
					model.add(route[selected][virtualEdge][physicalEdge]);
				}
			}
		}

		IloExpr routeCount(env);
		for (int selected = 0; selected < selectedCount; ++selected) {
			const int requestIndex = selectedRequests[selected];
			for (int virtualEdge = 0;
				virtualEdge < requests[requestIndex]->getGraph()->getM(); ++virtualEdge) {
				for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
					routeCount += route[selected][virtualEdge][physicalEdge];
				}
			}
		}
		model.add(IloMinimize(env, routeCount));
		routeCount.end();

		// Every virtual node is placed exactly once.
		for (int selected = 0; selected < selectedCount; ++selected) {
			const int requestIndex = selectedRequests[selected];
			Graph* virtualGraph = requests[requestIndex]->getGraph();
			for (int virtualNode = 0; virtualNode < virtualGraph->getN(); ++virtualNode) {
				IloExpr expression(env);
				for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
					if (!location || virtualGraph->getDist(virtualNode, physicalNode) <=
						requests[requestIndex]->getMaxD()) {
						expression += placement[selected][virtualNode][physicalNode];
					}
				}
				model.add(expression == 1);
				expression.end();
			}
		}

		// Nodes from the same request cannot share a physical node.
		for (int selected = 0; selected < selectedCount; ++selected) {
			const int requestIndex = selectedRequests[selected];
			Graph* virtualGraph = requests[requestIndex]->getGraph();
			for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
				IloExpr expression(env);
				for (int virtualNode = 0; virtualNode < virtualGraph->getN(); ++virtualNode) {
					if (!location || virtualGraph->getDist(virtualNode, physicalNode) <=
						requests[requestIndex]->getMaxD()) {
						expression += placement[selected][virtualNode][physicalNode];
					}
				}
				model.add(expression <= 1);
				expression.end();
			}
		}

		// Physical CPU capacities.
		for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
			IloExpr expression(env);
			for (int selected = 0; selected < selectedCount; ++selected) {
				const int requestIndex = selectedRequests[selected];
				Graph* virtualGraph = requests[requestIndex]->getGraph();
				for (int virtualNode = 0; virtualNode < virtualGraph->getN(); ++virtualNode) {
					if (!location || virtualGraph->getDist(virtualNode, physicalNode) <=
						requests[requestIndex]->getMaxD()) {
						expression += virtualGraph->getNodes()[virtualNode].getCPU() *
							placement[selected][virtualNode][physicalNode];
					}
				}
			}
			model.add(expression <= substrate->getNodes()[physicalNode].getCPU());
			expression.end();
		}

		// Physical bandwidth capacities. A route variable denotes that the
		// undirected substrate edge belongs to the selected path/subgraph.
		for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
			IloExpr expression(env);
			for (int selected = 0; selected < selectedCount; ++selected) {
				const int requestIndex = selectedRequests[selected];
				Graph* virtualGraph = requests[requestIndex]->getGraph();
				for (int virtualEdge = 0; virtualEdge < virtualGraph->getM(); ++virtualEdge) {
					expression += virtualGraph->getEdges()[virtualEdge].getBW() *
						route[selected][virtualEdge][physicalEdge];
				}
			}
			model.add(expression <= substrate->getEdges()[physicalEdge].getBW());
			expression.end();
		}

		// Seed connectivity with singleton cutsets. This removes the most common
		// disconnected solutions before the iterative separator starts.
		for (int selected = 0; selected < selectedCount; ++selected) {
			const int requestIndex = selectedRequests[selected];
			Graph* virtualGraph = requests[requestIndex]->getGraph();
			for (int virtualEdge = 0; virtualEdge < virtualGraph->getM(); ++virtualEdge) {
				const int originVirtual = virtualGraph->getEdges()[virtualEdge].getOrig();
				const int destinationVirtual = virtualGraph->getEdges()[virtualEdge].getDest();
				for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
					IloExpr incident(env);
					for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
						const Edge edge = substrate->getEdges()[physicalEdge];
						if (edge.getOrig() == physicalNode || edge.getDest() == physicalNode) {
							incident += route[selected][virtualEdge][physicalEdge];
						}
					}

					IloExpr difference(env);
					if (!location || virtualGraph->getDist(originVirtual, physicalNode) <=
						requests[requestIndex]->getMaxD()) {
						difference += placement[selected][originVirtual][physicalNode];
					}
					if (!location || virtualGraph->getDist(destinationVirtual, physicalNode) <=
						requests[requestIndex]->getMaxD()) {
						difference -= placement[selected][destinationVirtual][physicalNode];
					}

					model.add(incident >= difference);
					model.add(incident >= -difference);
					incident.end();
					difference.end();
				}
			}
		}

		IloCplex cplex(model);
		cplex.setOut(env.getNullStream());
		cplex.setWarning(env.getNullStream());
		cplex.setParam(IloCplex::Threads, 1);
		cplex.setParam(IloCplex::MIPDisplay, 0);

		const auto start = std::chrono::steady_clock::now();
		constexpr int maxConnectivityRounds = 5000;

		for (int round = 0; round < maxConnectivityRounds; ++round) {
			const double elapsed = std::chrono::duration<double>(
				std::chrono::steady_clock::now() - start).count();
			const double remaining = timeLimitSeconds - elapsed;
			if (remaining <= 0.0) {
				result = FeasibilityStatus::Unknown;
				break;
			}
			cplex.setParam(IloCplex::TiLim, remaining);

			if (!cplex.solve()) {
				result = cplex.getStatus() == IloAlgorithm::Infeasible
					? FeasibilityStatus::Infeasible
					: FeasibilityStatus::Unknown;
				break;
			}

			bool addedConnectivityCut = false;
			for (int selected = 0; selected < selectedCount; ++selected) {
				const int requestIndex = selectedRequests[selected];
				Graph* virtualGraph = requests[requestIndex]->getGraph();
				for (int virtualEdge = 0; virtualEdge < virtualGraph->getM(); ++virtualEdge) {
					const int originVirtual = virtualGraph->getEdges()[virtualEdge].getOrig();
					const int destinationVirtual = virtualGraph->getEdges()[virtualEdge].getDest();
					int originPhysical = -1;
					int destinationPhysical = -1;

					for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
						if ((!location || virtualGraph->getDist(originVirtual, physicalNode) <=
							requests[requestIndex]->getMaxD()) &&
							cplex.getValue(placement[selected][originVirtual][physicalNode]) > 0.5) {
							originPhysical = physicalNode;
						}
						if ((!location || virtualGraph->getDist(destinationVirtual, physicalNode) <=
							requests[requestIndex]->getMaxD()) &&
							cplex.getValue(placement[selected][destinationVirtual][physicalNode]) > 0.5) {
							destinationPhysical = physicalNode;
						}
					}

					std::vector<bool> reached(physicalNodes, false);
					std::vector<int> stack;
					reached[originPhysical] = true;
					stack.push_back(originPhysical);
					while (!stack.empty()) {
						const int node = stack.back();
						stack.pop_back();
						for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
							if (cplex.getValue(route[selected][virtualEdge][physicalEdge]) <= 0.5) {
								continue;
							}
							const Edge edge = substrate->getEdges()[physicalEdge];
							int neighbor = -1;
							if (edge.getOrig() == node) neighbor = edge.getDest();
							if (edge.getDest() == node) neighbor = edge.getOrig();
							if (neighbor >= 0 && !reached[neighbor]) {
								reached[neighbor] = true;
								stack.push_back(neighbor);
							}
						}
					}

					if (reached[destinationPhysical]) {
						continue;
					}

					IloExpr crossing(env);
					for (int physicalEdge = 0; physicalEdge < physicalEdges; ++physicalEdge) {
						const Edge edge = substrate->getEdges()[physicalEdge];
						if (reached[edge.getOrig()] != reached[edge.getDest()]) {
							crossing += route[selected][virtualEdge][physicalEdge];
						}
					}

					IloExpr endpointDifference(env);
					for (int physicalNode = 0; physicalNode < physicalNodes; ++physicalNode) {
						if (!reached[physicalNode]) continue;
						if (!location || virtualGraph->getDist(originVirtual, physicalNode) <=
							requests[requestIndex]->getMaxD()) {
							endpointDifference +=
								placement[selected][originVirtual][physicalNode];
						}
						if (!location || virtualGraph->getDist(destinationVirtual, physicalNode) <=
							requests[requestIndex]->getMaxD()) {
							endpointDifference -=
								placement[selected][destinationVirtual][physicalNode];
						}
					}

					model.add(crossing >= endpointDifference);
					model.add(crossing >= -endpointDifference);
					crossing.end();
					endpointDifference.end();
					addedConnectivityCut = true;
					break;
				}
				if (addedConnectivityCut) {
					break;
				}
			}

			if (!addedConnectivityCut) {
				result = FeasibilityStatus::Feasible;
				break;
			}
		}
	} catch (const IloException&) {
		result = FeasibilityStatus::Unknown;
	} catch (...) {
		result = FeasibilityStatus::Unknown;
	}

	env.end();
	return result;
}
