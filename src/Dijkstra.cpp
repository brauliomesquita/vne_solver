#include "Dijkstra.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

std::vector<int> Dijkstra::Run(double **bandwidth, Graph *substrate, int source,
    int destination, double requiredBandwidth) {
    const int nodeCount = substrate->getN();
    if (source < 0 || source >= nodeCount || destination < 0 || destination >= nodeCount) {
        return {};
    }

    using QueueEntry = std::pair<double, int>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
    std::vector<double> distance(nodeCount, std::numeric_limits<double>::infinity());
    std::vector<int> previous(nodeCount, -1);

    distance[source] = 0.0;
    queue.emplace(0.0, source);

    while (!queue.empty()) {
        const double currentDistance = queue.top().first;
        const int current = queue.top().second;
        queue.pop();

        if (currentDistance != distance[current]) {
            continue;
        }
        if (current == destination) {
            break;
        }

        for (int next = 0; next < nodeCount; ++next) {
            const int edge = substrate->getAdj(current, next);
            if (edge < 0 || bandwidth[current][next] < requiredBandwidth) {
                continue;
            }

            const double candidate = currentDistance + substrate->getCost(edge);
            if (candidate < distance[next]) {
                distance[next] = candidate;
                previous[next] = current;
                queue.emplace(candidate, next);
            }
        }
    }

    if (!std::isfinite(distance[destination])) {
        return {};
    }

    std::vector<int> path;
    for (int node = destination; node != -1; node = previous[node]) {
        path.push_back(node);
    }
    std::reverse(path.begin(), path.end());
    return path;
}
