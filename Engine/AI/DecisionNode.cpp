#include "DecisionNode.h"

//Constructor
DecisionNode::DecisionNode(std::function<bool()> qns,
DecisionNode* trueNode, DecisionNode* falseNode,
std::function<void()> leafAction):mainqns(qns), 
ifTrue(trueNode),ifFalse(falseNode),action(leafAction){};

void DecisionNode::evaluate()
{
    if (mainqns) {
        if (mainqns()) {
            if (ifTrue) ifTrue->evaluate();
            else if (action) action();
        }
        else {
            if (ifFalse) ifFalse->evaluate();
            else if (action) action();
        }
    }
    else if (action) {
        action();
    }
}