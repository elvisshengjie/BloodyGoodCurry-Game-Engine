#pragma once
#include "../AI/DecisionTree.h"
#include "../AI/Blackboard.h"
#include "Composition/Component.h"
#include "Component/HitBoxComponent.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
namespace Framework
{
    class BehaviorTreeComponent : public GameComponent
    {
        public:
            using TreeBuilder = std::function<std::unique_ptr<DecisionTree>(GOC*)>;
            static std::unordered_map<std::string, TreeBuilder>& Registry()
            {
                static std::unordered_map<std::string, TreeBuilder> reg;
                return reg;
            }
            std::string treeType;
            std::unique_ptr<DecisionTree> tree;
            std::unique_ptr<BlackBoard> blackboard;
            std::function<void(GOC*, float, float, float, float, float, float, float)> spawnHitBoxFn;
            std::function<void(GOC*, float, float, float, float, float, float, float, float, float)> spawnProjectileFn;

            void initialize() override
            {
                blackboard = std::make_unique<BlackBoard>();
            }
            void BuildTree(GOC* goc)
            {
                tree.reset();
                auto it = Registry().find(treeType);
                if (it != Registry().end())
                    tree = it->second(goc);
            }
            void Update(float dt, GOC* treeOwner)
            {
                if (!tree) return;
                if (!blackboard) blackboard = std::make_unique<BlackBoard>();
                BehaviorContext ctx;
                ctx.dt = dt;
                ctx.owner = treeOwner;
                ctx.blackboard = blackboard.get();
                ctx.spawnHitBox = spawnHitBoxFn;
                ctx.spawnProjectile = spawnProjectileFn;
                tree->run(ctx);
            }
            
            ComponentHandle Clone() const override
            {
                auto copy = ComponentPool<BehaviorTreeComponent>::CreateTyped();
                copy->treeType = treeType;
                // Do NOT clone tree or blackboard — they get built fresh via BuildTree
                return copy;
            }
    };
}