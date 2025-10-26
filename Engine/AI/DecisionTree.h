#pragma once
#include "../AI/DecisionNode.h"
class DecisionTree
{
    public:
     DecisionTree(DecisionNode*startNode);
     void run();
     private:
     DecisionNode*rootNode;
     enum ID{Behavior1};
};