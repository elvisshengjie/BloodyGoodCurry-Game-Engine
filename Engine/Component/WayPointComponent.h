#pragma once
#include <vector>
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"

namespace Framework
{
    class WayPointComponent : public GameComponent
    {
    public:
        std::vector<int> NeighborIDs;  // IDs of neighboring waypoints

        // Component lifecycle
        void initialize() override {}
        void SendMessage(Message& m) override { (void)m; }

        // Serialization
        void Serialize(ISerializer& s) override
        {
            // Safely read the NeighborIDs array from the serializer
            StreamRead(s, "NeighborIDs", NeighborIDs);
        }

        // Clone for creating deep copies
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<WayPointComponent>::CreateTyped();
            if (copy)
                copy->NeighborIDs = NeighborIDs;  // copy neighbors
            return copy;
        }
    };
}
