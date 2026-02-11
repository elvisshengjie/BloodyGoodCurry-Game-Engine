#pragma once
namespace Framework
{
	struct SearchNode
	{
		int NodeID;
		int ParentID;
		float ActualCost;
		float EstimatedCost;
		float TotalCost;
		SearchNode() :
			NodeID(0),
			ParentID(0),
			ActualCost(0),
			EstimatedCost(0),
			TotalCost(0)
		{}
		
		SearchNode(int ID, int Parent, float gCost, float hCost) :
			NodeID(ID),
			ParentID(Parent),
			ActualCost(gCost),
			EstimatedCost(hCost),
			TotalCost(gCost + hCost)
		{}

		void UpdateCosts(float gCost, float hCost)
		{
			ActualCost = gCost;
			EstimatedCost = hCost;
			TotalCost = gCost + hCost;
		}

		bool operator>(const SearchNode& other) const
		{return TotalCost > other.TotalCost;}
		bool operator<(const SearchNode& other) const
		{return TotalCost < other.TotalCost;}
	};
}