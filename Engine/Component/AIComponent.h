#pragma once
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include <vector>
namespace Framework
{
    class AIComponent : public GameComponent
    {
        public:
        std::vector<int> currentPathNodeIDs;
        size_t currentPathIndex = 0;
        float targetX = 0.0f;
        float targetY = 0.0f;
        void initialize() override {}
        void SendMessage(Message& m) override{(void)m;}
        void Serialize(ISerializer& s) override{(void)s;}
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<AIComponent>::CreateTyped();
            copy->currentPathNodeIDs = currentPathNodeIDs;
            copy->currentPathIndex = currentPathIndex;
            copy->targetX = targetX;
            copy->targetY = targetY;
            return copy;
        }
        void ClearPath()
        {currentPathNodeIDs.clear();currentPathIndex = 0;}
        bool HasPath() const
        {return !currentPathNodeIDs.empty() && currentPathIndex < currentPathNodeIDs.size();}
    };
}
