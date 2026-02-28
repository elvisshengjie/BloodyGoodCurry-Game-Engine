#pragma once

#include <string>

#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"

namespace Framework {

    class BehaviourComponent : public GameComponent
    {
    public:
        std::string behaviourKey{};
        bool started{ false };

        void Serialize(ISerializer& s) override
        {
            started = false;
            if (s.HasKey("behaviourKey"))
            {
                StreamRead(s, "behaviourKey", behaviourKey);
            }
        }

        ComponentHandle Clone() const override
        {
            return ComponentPool<BehaviourComponent>::Create(*this);
        }
    };

}
