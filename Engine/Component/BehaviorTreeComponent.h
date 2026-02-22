#pragma once
#include "../AI/DecisionTree.h"
#include "../AI/Blackboard.h"
#include "Composition/Component.h"
#include <memory>

namespace Framework
{
    
    class BehaviorTreeComponent : public GameComponent
    {
    public:
        std::unique_ptr<DecisionTree> tree;
        std::unique_ptr<BlackBoard> blackboard;

        void initialize() override
        {
            tree.reset();
            blackboard.reset();
        }

        void Update(float dt, GOC* treeOwner)
        {
            if (!tree) return;
            if (!blackboard) blackboard = std::make_unique<BlackBoard>();
            BehaviorContext ctx;
            ctx.dt = dt;
            ctx.owner = treeOwner;
            ctx.blackboard = blackboard.get();
            tree->run(ctx);
        }
    };
}