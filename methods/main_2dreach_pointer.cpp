#include "../containers/graph.h"
#include "../containers/rtree_statistics.hpp"
#include "../def_global.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <cstdio>
#include <optional>

namespace {
    using Point2D = bg::model::point<float, 2, bg::cs::cartesian>;
    using RTree = bgi::rtree<Point2D, bgi::quadratic<16>>;

#ifdef BOOST_GEOMETRIES
    Point2D toPoint(const Node &node)
    {
        return Point2D(bg::get<0>(node.geometry), bg::get<1>(node.geometry));
    }
#else
    Point2D toPoint(const Node &node)
    {
        return Point2D(node.geometry.x, node.geometry.y);
    }
#endif

}

struct SubGraph
{
    vector<optional<ID>> compToSubgraphComp;
    vector<AdjacencyList> adjs;
    vector<size_t> componentIndegree;
    vector<size_t> componentOutdegree;
};

vector<ID> topo_sort_kahn(const SubGraph& g) {
    const size_t n = g.adjs.size();
    vector<size_t> indeg = g.componentIndegree; // work copy (don’t mutate input)
    queue<ID> q;

    for (ID u = 0; u < n; ++u) {
        if (indeg[u] == 0) q.push(u);
    }

    vector<ID> order;
    order.reserve(n);

    while (!q.empty()) {
        ID u = q.front(); q.pop();
        order.push_back(u);
        for (ID v : g.adjs[u]) {
            if (--indeg[v] == 0) q.push(v);
        }
    }

    if (order.size() != n) return {};
    reverse(order.begin(), order.end()); // sinks-first
    return order;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << endl;
        cerr << "Usage: " << argv[0] << " INPUT_PREFIX QUERY_FILE" << endl;
        cerr << endl;
        return 1;
    }

    const string prefix      = string(argv[1]);
    const string fileQueries = string(argv[2]);

    GeosocialDAGraph G(prefix);

    const size_t numNodes = G.numNodes;
    const ID numComponents = G.numComponents;

    Timer qtimer;
    qtimer.start();

    vector<ID> nonSpatialComponentIDs;
    for (const Component &comp : G.components) {
        if (!comp.isSpatial) {
            nonSpatialComponentIDs.push_back(comp.id);
        } else {
            assert(comp.nodes.size() == 1);
        }
    }

    SubGraph subgraph;
    subgraph.compToSubgraphComp.resize(numNodes, std::nullopt);
    for (size_t i = 0; i < nonSpatialComponentIDs.size(); i++) {
        subgraph.compToSubgraphComp[nonSpatialComponentIDs[i]] = i;
    }
    subgraph.adjs.resize(nonSpatialComponentIDs.size());
    subgraph.componentIndegree.resize(nonSpatialComponentIDs.size());
    subgraph.componentOutdegree.resize(nonSpatialComponentIDs.size());

    for (const ID cid : nonSpatialComponentIDs) {
        for (const ID child : G.cadjs[cid]) {
            if (!G.components[child].isSpatial) {
                const ID sub_cid = subgraph.compToSubgraphComp[cid].value();
                const ID sub_child = subgraph.compToSubgraphComp[child].value();
                subgraph.adjs[sub_cid].push_back(sub_child);
                subgraph.componentOutdegree[sub_cid]++;
                subgraph.componentIndegree[sub_child]++;
            }
        }
    }

    vector<size_t> componentSpatialCounts(numComponents, 0);
    for (ID nid = 0; nid < numNodes; ++nid) {
        const Node &node = G.nodes[nid];
        if (node.isSpatial) {
            ++componentSpatialCounts[node.cid];
        }
    }

    vector<vector<Point2D>> componentAggregated(numComponents);
    for (ID cid = 0; cid < numComponents; ++cid) {
        componentAggregated[cid].reserve(componentSpatialCounts[cid]);
    }

    for (ID nid = 0; nid < numNodes; ++nid) {
        const Node &node = G.nodes[nid];
        if (node.isSpatial) {
            componentAggregated[node.cid].emplace_back(toPoint(node));
        }
    }

    const vector<ID> order = topo_sort_kahn(subgraph);

    vector<shared_ptr<RTree>> componentRTrees(nonSpatialComponentIDs.size());
    for (const ID orderId : order) {
        const ID cid = nonSpatialComponentIDs[orderId];

        auto &aggregatedEntries = componentAggregated[cid];

        size_t biggestSize = 0;
        ID biggestChild;
        for (ID child : G.cadjs[cid]) {
            auto &childEntries = componentAggregated[child];
            if (childEntries.empty()) {
                continue;
            }

            if (!G.components[child].isSpatial && childEntries.size() > biggestSize) {
                biggestSize = childEntries.size();
                biggestChild = child;
            }

            aggregatedEntries.reserve(childEntries.size());
            aggregatedEntries.insert(aggregatedEntries.end(), childEntries.begin(), childEntries.end());
        }

        if (biggestSize > 0 && biggestSize == aggregatedEntries.size()) {
            componentRTrees[orderId] = componentRTrees[subgraph.compToSubgraphComp[biggestChild].value()];
        } else if (!aggregatedEntries.empty()) {
            componentRTrees[orderId] = make_shared<RTree>(aggregatedEntries.begin(), aggregatedEntries.end());
        }
    }

    vector<const RTree *> rTreeForNonSpatialComponent(nonSpatialComponentIDs.size(), nullptr);
    const size_t N = G.components.size();

    std::vector<uint64_t> spatialMask((N + 63) / 64, 0);

    std::vector<uint32_t> rankPrefix(spatialMask.size());
    for (const ID orderId : order) {
        ID compId = nonSpatialComponentIDs[orderId];

        const size_t word = compId >> 6;
        const size_t bit  = compId & 63;

        spatialMask[word] |= (1ULL << bit);
        rTreeForNonSpatialComponent[orderId] = componentRTrees[orderId].get();
    }
    uint32_t sum = 0;
    for (size_t i = 0; i < spatialMask.size(); ++i)
    {
        rankPrefix[i] = sum;
        sum += __builtin_popcountll(spatialMask[i]);
    }

    const double timeIndexing = qtimer.stop();

    size_t totalRTreeBytes   = 0;
    size_t totalRTreeEntries = 0;
    for (const ID orderId : order)
    {
        const auto &treePtr = componentRTrees[orderId];
        if (!treePtr)
            continue;

        auto stats = bgi::detail::rtree::utilities::statistics(*treePtr);
        totalRTreeBytes += get<6>(stats);
        totalRTreeEntries += treePtr->size();
    }
    size_t rTreeForNodeBytes = nonSpatialComponentIDs.size() * sizeof(const RTree *) + spatialMask.size() * sizeof(uint64_t) + rankPrefix
    .size() * sizeof(uint32_t);


    // STEP 3: answer RangeReach queries
    ifstream inp(fileQueries);
    if (!inp)
    {
        cerr << endl
                  << "Error: cannot open queries file \"" << fileQueries << "\"" << endl
                  << endl;
        return 1;
    }

    size_t numQueries = 0;
    size_t numTrues   = 0;
    double timeQuerying    = 0.0;

    ID nid;
    double xlow, ylow, xhigh, yhigh;

    while (inp >> nid >> xlow >> ylow >> xhigh >> yhigh) {
        bool qres = false;
        Timer qtimerQuery;
        qtimerQuery.start();

        const ID cid = G.nodes[nid].cid;

        const size_t word = cid >> 6;
        const size_t bit  = cid & 63;

        uint64_t mask = spatialMask[word];
        const RTree *treePtr;
        if ((mask & (1ULL << bit)) == 0) {
            treePtr = nullptr;
        } else {
            uint32_t rank =
                rankPrefix[word] +
                __builtin_popcountll(mask & ((1ULL << bit) - 1));

            treePtr = rTreeForNonSpatialComponent[rank];
        }

        if (treePtr) {
            bg::model::box queryWindow(Point2D(static_cast<float>(xlow), static_cast<float>(ylow)),
                                                Point2D(static_cast<float>(xhigh), static_cast<float>(yhigh)));
            if (auto it = treePtr->qbegin(bgi::intersects(queryWindow)); it != treePtr->qend()) {
                qres = true;
            }
        } else {
            const Node n = G.nodes[nid];
            const float nx = n.geometry.x;
            const float ny = n.geometry.y;
            if (nx >= xlow && nx <= xhigh && ny >= ylow && ny <= yhigh) {
                qres = true;
            }
        }

        timeQuerying += qtimerQuery.stop();
        ++numQueries;
        if (qres)
            ++numTrues;
    }
    inp.close();

    cout << endl;
    cout << "Report" << endl;
    cout << "======" << endl << endl;
    cout << "Input prefix             : " << prefix << endl;
    cout << "Query file               : " << fileQueries << endl;
    cout << "Method                   : 2DReach-Pointer" << endl << endl;
    printf("Indexing time [secs]     : %.10lf\n", timeIndexing);
    cout << "Index size [Bytes]       : " << totalRTreeBytes + rTreeForNodeBytes << " (" << rTreeForNodeBytes << ")" << endl;
    cout << "Num of queries           : " << numQueries << endl;
    cout << "Num of true results      : " << numTrues << endl;
    if (numQueries > 0)
        printf("Avg query time [secs]    : %.10lf\n", timeQuerying / numQueries);
    else
        cout << "Avg query time [secs]    : 0" << endl;
    cout << endl;

    return 0;
}