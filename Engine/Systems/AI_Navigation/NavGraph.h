#pragma once
#include "GraphNode.h"
#include "SearchNode.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <algorithm>
namespace Framework
{
	class NavGraph
	{
		private:
			std::unordered_map<int, GraphNode> nodes;
			float CalculateHeurisitic(const GraphNode& from, const GraphNode& to) const
			{
				float dx = to.x - from.x;
				float dy= to.y - from.y;
				return std::sqrt(dx * dx + dy * dy);
			}
			float CalculateEdgeCost(const GraphNode& from, const GraphNode& to) const
			{return CalculateHeurisitic(from, to);}

		public:
			NavGraph() = default;
			~NavGraph() = default;

			void AddNode(int id, float x, float y)
			{nodes[id] = GraphNode(id, x, y);}

			void RemoveNode(int id)
			{
				if (nodes.find(id) != nodes.end())
				{
					const auto& nodeToRemove = nodes[id];
					for (int neighborID : nodeToRemove.NeighborIDs)
					{
						if (nodes.find(neighborID) != nodes.end())
						{nodes[neighborID].RemoveNeighbor(id);}
					}
				}
				nodes.erase(id);
			}

			void AddDirectedEdge(int fromID, int toID)
			{
				if (nodes.find(fromID) != nodes.end() && nodes.find(toID) != nodes.end())
					nodes[fromID].AddNeighbour(toID);
			}
			
			const GraphNode* GetNode(int id) const
			{
				auto it = nodes.find(id);
				return (it != nodes.end()) ? &it->second : nullptr;
			}
			
			int FindNearestNode(float x, float y) const
			{
				if (nodes.empty()) return 0;
				int nearestID = 0;
				float minDistance = std::numeric_limits<float>::max();
				for (const auto& pair : nodes)
				{
					const GraphNode& node = pair.second;
					float dx = node.x - x;
					float dy = node.y - y;
					float distance = dx * dx + dy * dy;
					if (distance < minDistance)
					{
						minDistance = distance;
						nearestID = node.ID;
					}
				}
				return nearestID;
			}

            std::vector<int> FindPath(int startID, int goalID)
            {
                std::vector<int> path;
                if (nodes.find(startID) == nodes.end() || nodes.find(goalID) == nodes.end())
                   return path;  
                if (startID == goalID)
                {
                    path.push_back(startID);
                    return path;
                }
                auto compare = [](const SearchNode& a, const SearchNode& b) { return a > b; };
                std::priority_queue<SearchNode, std::vector<SearchNode>, decltype(compare)> openList(compare);
                std::unordered_map<char, bool> closedList;
                std::unordered_map<char, float> gScores;
                std::unordered_map<char, char> cameFrom;
                const GraphNode& startNode = nodes[startID];
                const GraphNode& goalNode = nodes[goalID];
                float startH = CalculateHeurisitic(startNode, goalNode);
                SearchNode startSearch(startID, 0, 0.0f, startH);
                openList.push(startSearch);
                gScores[startID] = 0.0f;
                cameFrom[startID] = 0;  // No parent

                while (!openList.empty())
                {
                    SearchNode current = openList.top();
                    openList.pop();
                    if (closedList[current.NodeID])
                        continue;
                    closedList[current.NodeID] = true;
                    if (current.NodeID == goalID)
                    {
                        char pathNode = goalID;
                        while (pathNode != 0)  // 0 indicates no parent
                        {
                            path.push_back(pathNode);
                            pathNode = cameFrom[pathNode];
                        }
                        std::reverse(path.begin(), path.end());
                        return path;
                    }
                    const GraphNode& currentNode = nodes[current.NodeID];
                    for (char neighborID : currentNode.NeighborIDs)
                    {
                        if (closedList[neighborID])
                        {continue;}
                        const GraphNode& neighborNode = nodes[neighborID];
                        float tentativeG = current.ActualCost + CalculateEdgeCost(currentNode, neighborNode);
                        if (gScores.find(neighborID) == gScores.end() || tentativeG < gScores[neighborID])
                        {
                            gScores[neighborID] = tentativeG;
                            cameFrom[neighborID] = current.NodeID;
                            float h = CalculateHeurisitic(neighborNode, goalNode);
                            SearchNode neighborSearch(neighborID, current.NodeID, tentativeG, h);

                            openList.push(neighborSearch);
                        }
                    }
                }
                return path;
            }

			size_t GetNodeCount() const { return nodes.size(); }
			
			void Clear() { nodes.clear();}

			std::vector<int> GetAllNodesIDs() const
			{
				std::vector<int> ids;
				ids.reserve(nodes.size());
				for (const auto& pair : nodes)
				{ids.push_back(pair.first);}
				return ids;
			}

	};
}