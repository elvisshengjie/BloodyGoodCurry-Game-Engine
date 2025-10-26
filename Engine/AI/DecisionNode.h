#pragma once
#include <memory>
#include <functional>
class DecisionNode
{
    public:
     std::function<bool()> mainqns;
     std::unique_ptr<DecisionNode> ifTrue;
     std::unique_ptr<DecisionNode> ifFalse;
     std::function<void()> action;
     
     DecisionNode(std::function<bool()> qns,
     DecisionNode* trueNode = nullptr, DecisionNode* falseNode = nullptr,
     std::function<void()> leafAction = nullptr);

     void evaluate();

};

