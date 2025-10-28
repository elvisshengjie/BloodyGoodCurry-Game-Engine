#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "AI/DecisionTree.h"
#include <Windows.h>
#ifdef SendMessage
    #undef SendMessage
#endif

namespace Framework {

class EnemyDecisionTreeComponent : public GameComponent 
{
 public:
    std::unique_ptr<DecisionTree> tree;
    void initialize() override {}   
    void SendMessage(Message& m) override { (void)m;}
    void Serialize(ISerializer& s) override { (void)s; }
    std::unique_ptr<GameComponent> Clone() const override 
    {
        auto copy = std::make_unique<EnemyDecisionTreeComponent>();
        if (tree) copy->tree = std::make_unique<DecisionTree>(*tree); // deep copy
        return copy;
    }
};

} 
