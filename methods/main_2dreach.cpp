#include "../containers/graph.h"
#include "../containers/rtree_statistics.hpp"
#include "../def_global.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace
{

using Point2D = bg::model::point<float, 2, bg::cs::cartesian>;
using RangeTree = bgi::rtree<Point2D, bgi::quadratic<16>>;

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

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << std::endl;
        std::cerr << "Usage: " << argv[0] << " INPUT_PREFIX QUERY_FILE" << std::endl;
        std::cerr << std::endl;
        return 1;
    }

    const std::string prefix      = std::string(argv[1]);
    const std::string fileQueries = std::string(argv[2]);

    GeosocialDAGraph G(prefix);

    const ID numNodes      = G.numNodes;
    const ID numComponents = G.numComponents;

    Timer timerIndex;
    timerIndex.start();

    const std::vector<AdjacencyList> &componentChildren = G.cadjs;

    std::vector<std::size_t> indegreeCopy(numComponents, 0);
    std::queue<ID> topoQueue;
    for (const Component &comp : G.components)
    {
        const std::size_t indegree = static_cast<std::size_t>(comp.indegree);
        indegreeCopy[comp.id]       = indegree;
        if (indegree == 0)
            topoQueue.push(comp.id);
    }

    const double timeSCC = 0.0;

    std::vector<std::size_t> componentSpatialCounts(numComponents, 0);
    for (ID nid = 0; nid < numNodes; ++nid)
    {
        const Node &node = G.nodes[nid];
        if (node.isSpatial)
            ++componentSpatialCounts[node.cid];
    }

    std::vector<std::vector<Point2D>> componentAggregated(numComponents);
    for (ID cid = 0; cid < numComponents; ++cid)
        componentAggregated[cid].reserve(componentSpatialCounts[cid]);

    for (ID nid = 0; nid < numNodes; ++nid)
    {
        const Node &node = G.nodes[nid];
        if (node.isSpatial)
            componentAggregated[node.cid].emplace_back(toPoint(node));
    }

    std::vector<ID> topoOrder;
    topoOrder.reserve(numComponents);

    while (!topoQueue.empty())
    {
        ID cid = topoQueue.front();
        topoQueue.pop();
        topoOrder.push_back(cid);

        for (ID child : componentChildren[cid])
        {
            if (--indegreeCopy[child] == 0)
                topoQueue.push(child);
        }
    }

    if (topoOrder.size() != static_cast<std::size_t>(numComponents))
    {
        std::cerr << std::endl
                  << "Error: condensation graph contains a cycle" << std::endl
                  << std::endl;
        return 1;
    }

    std::vector<std::size_t> componentDescendantCounts = componentSpatialCounts;
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it)
    {
        ID cid = *it;
        std::size_t total = componentDescendantCounts[cid];
        for (ID child : componentChildren[cid])
            total += componentDescendantCounts[child];
        componentDescendantCounts[cid] = total;
    }

    std::vector<std::unique_ptr<RangeTree>> componentRangeTrees(numComponents);

    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it)
    {
        ID cid = *it;
        auto &aggregatedEntries = componentAggregated[cid];

        if (componentDescendantCounts[cid] > aggregatedEntries.capacity())
            aggregatedEntries.reserve(componentDescendantCounts[cid]);

        for (ID child : componentChildren[cid])
        {
            auto &childEntries = componentAggregated[child];
            if (childEntries.empty())
                continue;

            if (static_cast<std::size_t>(G.components[child].indegree) == 1)
            {
                if (aggregatedEntries.empty())
                {
                    aggregatedEntries = std::move(childEntries);
                    if (componentDescendantCounts[cid] > aggregatedEntries.capacity())
                        aggregatedEntries.reserve(componentDescendantCounts[cid]);
                    continue;
                }
                else
                {
                    if (aggregatedEntries.size() < childEntries.size())
                    {
                        aggregatedEntries.swap(childEntries);
                        if (componentDescendantCounts[cid] > aggregatedEntries.capacity())
                            aggregatedEntries.reserve(componentDescendantCounts[cid]);
                    }

                    aggregatedEntries.insert(aggregatedEntries.end(),
                                            std::make_move_iterator(childEntries.begin()),
                                            std::make_move_iterator(childEntries.end()));
                    childEntries.clear();
                }
            }
            else
            {
                aggregatedEntries.insert(aggregatedEntries.end(), childEntries.begin(), childEntries.end());
            }
        }

        if (componentDescendantCounts[cid] > aggregatedEntries.capacity())
            aggregatedEntries.reserve(componentDescendantCounts[cid]);

        if (!aggregatedEntries.empty())
        {
            componentRangeTrees[cid] = std::make_unique<RangeTree>(aggregatedEntries.begin(), aggregatedEntries.end());
        }
    }

    std::vector<const RangeTree *> rangeTreeForNode(numNodes, nullptr);
    for (ID nid = 0; nid < numNodes; ++nid)
    {
        const auto &treePtr = componentRangeTrees[G.nodes[nid].cid];
        if (treePtr)
            rangeTreeForNode[nid] = treePtr.get();
    }

    double timeIndexing = timerIndex.stop();

    std::size_t totalRangeTreeBytes   = 0;
    std::size_t totalRangeTreeEntries = 0;
    for (ID cid = 0; cid < numComponents; ++cid)
    {
        const auto &treePtr = componentRangeTrees[cid];
        if (!treePtr)
            continue;

        auto stats = bgi::detail::rtree::utilities::statistics(*treePtr);
        totalRangeTreeBytes += std::get<6>(stats);
        totalRangeTreeEntries += treePtr->size();
    }
    std::size_t rangeTreeForNodeBytes = rangeTreeForNode.size() * sizeof(const RangeTree *);

    for (ID nid = 0; nid < numNodes; ++nid)
    {
        const auto &treePtr = componentRangeTrees[G.nodes[nid].cid];
        if (treePtr)
            rangeTreeForNode[nid] = treePtr.get();
    }

    std::ifstream inp(fileQueries);
    if (!inp)
    {
        std::cerr << std::endl
                  << "Error: cannot open queries file \"" << fileQueries << "\"" << std::endl
                  << std::endl;
        return 1;
    }

    std::size_t numQueries = 0;
    std::size_t numTrues   = 0;
    double timeQuerying    = 0.0;

    ID nid;
    double xlow, ylow, xhigh, yhigh;

    while (inp >> nid >> xlow >> ylow >> xhigh >> yhigh)
    {
        if (nid >= numNodes)
        {
            std::cerr << std::endl
                      << "Error: query references invalid node identifier " << nid << std::endl
                      << std::endl;
            return 1;
        }

        bool qres = false;
        Timer qtimer;
        qtimer.start();

        const RangeTree *treePtr = rangeTreeForNode[nid];
        if (treePtr)
        {
            bg::model::box<Point2D> queryWindow(Point2D(static_cast<float>(xlow), static_cast<float>(ylow)),
                                                Point2D(static_cast<float>(xhigh), static_cast<float>(yhigh)));
            auto it = treePtr->qbegin(bgi::intersects(queryWindow));
            if (it != treePtr->qend())
                qres = true;
        }

        timeQuerying += qtimer.stop();
        ++numQueries;
        if (qres)
            ++numTrues;
    }
    inp.close();

    std::cout << std::endl;
    std::cout << "Report" << std::endl;
    std::cout << "======" << std::endl << std::endl;
    std::cout << "Input prefix             : " << prefix << std::endl;
    std::cout << "Query file               : " << fileQueries << std::endl;
    std::cout << "Method                   : 2DReach" << std::endl << std::endl;
    printf("Indexing time [secs]     : %.10lf\n", timeIndexing);
    std::cout << "Index size [Bytes]       : " << totalRangeTreeBytes + rangeTreeForNodeBytes << std::endl;
    std::cout << "SCC reuse time [secs]    : " << timeSCC << std::endl;
    std::cout << "Num of SCCs              : " << numComponents << std::endl;
    std::cout << "Total stored points      : " << totalRangeTreeEntries << std::endl << std::endl;
    std::cout << "Num of queries           : " << numQueries << std::endl;
    std::cout << "Num of true results      : " << numTrues << std::endl;
    if (numQueries > 0)
        printf("Avg query time [secs]    : %.10lf\n", timeQuerying / numQueries);
    else
        std::cout << "Avg query time [secs]    : 0" << std::endl;
    std::cout << std::endl;

    return 0;
}
