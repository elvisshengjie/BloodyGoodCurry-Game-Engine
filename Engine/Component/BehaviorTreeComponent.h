/*********************************************************************************************
 \file      BehaviorTreeComponent.h
 \par       SofaSpuds
 \author
 \brief     Declares the component that owns and drives a runtime AI decision tree.
 \details   Stores the selected tree type, runtime decision tree instance, per-object
            blackboard state, and gameplay callback hooks used by AI behaviors.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once
#include "../AI/DecisionTree.h"
#include "../AI/Blackboard.h"
#include "Composition/Component.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <iostream>
namespace Framework
{
    class BehaviorTreeComponent : public GameComponent
    {
        public:
            using TreeBuilder = std::function<std::unique_ptr<DecisionTree>(GOC*)>;

            /*************************************************************************************
             \brief  Returns the global registry of named decision-tree builders.
             \return A map from tree type name to builder callback.
            *************************************************************************************/
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

            /*************************************************************************************
             \brief  Initializes the component's per-instance blackboard.
            *************************************************************************************/
            void initialize() override
            {
                blackboard = std::make_unique<BlackBoard>();
            }

            /*************************************************************************************
             \brief  Rebuilds the runtime tree from the registered builder for the current type.
             \param  goc  The owning game object passed into the tree builder.
            *************************************************************************************/
            void BuildTree(GOC* goc)
            {
                tree.reset();
                auto it = Registry().find(treeType);
                if (it != Registry().end())
                    tree = it->second(goc);
                else
                    std::cout << "[AiSystem] BuildTree failed: treeType '" << treeType
                    << "' not found in registry size: " << Registry().size() << "\n";
            }

            /*************************************************************************************
             \brief  Updates the decision tree using the current behavior context.
             \param  dt        Fixed-step delta time in seconds.
             \param  treeOwner The owning game object executing the behavior tree.
            *************************************************************************************/
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

            /*************************************************************************************
             \brief  Loads the serialized tree type for this component.
             \param  stream  The serializer used to read component data.
            *************************************************************************************/
            void Serialize(ISerializer& stream) override
            {
                if (stream.HasKey("treeType"))
                    StreamRead(stream, "treeType", treeType);
            }

            /*************************************************************************************
             \brief  Creates a shallow copy of the component for duplication.
             \details The tree type is copied, while the runtime tree and blackboard are
                      rebuilt later rather than cloned directly.
             \return A cloned component handle.
            *************************************************************************************/
            ComponentHandle Clone() const override
            {
                auto copy = ComponentPool<BehaviorTreeComponent>::CreateTyped();
                copy->treeType = treeType;
                // Do NOT clone tree or blackboard — they get built fresh via BuildTree
                return copy;
            }


    };
}
