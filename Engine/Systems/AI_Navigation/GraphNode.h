#pragma once
#include <vector>
namespace Framework
{
	struct GraphNode
	{
		int ID;
		float x, y;
		std::vector<int> NeighborIDs;
		GraphNode() : ID(0), x(0.0f), y(0.0f) {}
		GraphNode(int ID, float posX, float posY) :ID(ID), x(posX), y(posY) {}
		void AddNeighbour(int NeighborID)
		{
			for (int nid : NeighborIDs) 
			{if (nid == NeighborID) return;}
			NeighborIDs.push_back(NeighborID);
		}
		void RemoveNeighbor(int NeighborID)
		{
			for (size_t i = 0; i < NeighborIDs.size(); i++)
			{
				if (NeighborIDs[i] == NeighborID)
				{
					NeighborIDs.erase(NeighborIDs.begin() + i);
					return;
				}
			}
		}
		bool IsConnected(int NeighborID) const
		{
			for (int nid : NeighborIDs)
			{
				if (nid == NeighborID) 
					return true;
			}
			return false;
		}
	};
}