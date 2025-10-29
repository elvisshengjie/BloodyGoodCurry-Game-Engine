#include "DecisionTree.h"
DecisionTree::DecisionTree(DecisionNode*startNode):rootNode(startNode){}
void DecisionTree::run(){if (rootNode){rootNode->evaluate();}}

