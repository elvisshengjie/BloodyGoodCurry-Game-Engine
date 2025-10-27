#pragma once
#include "Composition/Component.h"
#include "AI/DecisionTree.h"

namespace Framework {

class EnemyDecisionTreeComponent : public GameComponent {
public:
    std::unique_ptr<DecisionTree> tree;

    void initialize() override {}   
    

    void SendMessage(Message&) override {}

    void Serialize(ISerializer& s) override {}

    std::unique_ptr<GameComponent> Clone() const override 
    {
        auto copy = std::make_unique<EnemyDecisionTreeComponent>();
        if (tree) copy->tree = std::make_unique<DecisionTree>(*tree); // deep copy
        return copy;
    }
};

} 
