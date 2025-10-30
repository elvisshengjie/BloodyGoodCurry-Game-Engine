#include "DecisionNode.h"

//Constructor
DecisionNode::DecisionNode(std::function<bool()> qns,
 std::unique_ptr<DecisionNode> trueNode,
 std::unique_ptr<DecisionNode> falseNode,
 std::function<void()> leafAction)
 : mainqns(std::move(qns)), ifTrue(std::move(trueNode)), ifFalse(std::move(falseNode)), action(std::move(leafAction)) {}

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