/*********************************************************************************************
 \file      BehaviourComponent.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares BehaviourComponent, a lightweight component that binds a game object
            to a registered behaviour/script via a string key and tracks whether the
            behaviour has been started.

 \details
            BehaviourComponent stores:
            - behaviourKey: The identifier used by the behaviour/logic system to locate
              and execute the correct behaviour implementation for this object.
            - started: Runtime flag used to ensure a behaviour's "start/enter" logic runs
              once (typically reset on load/deserialize).
            The component supports serialization and cloning through the engine's
            component pool system.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include <string>

#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"

namespace Framework {

    /*****************************************************************************************
      \class BehaviourComponent
      \brief Component that associates a game object with a behaviour using a lookup key.

      \details
      This component enables data-driven behaviour assignment:
      - Levels/prefabs specify a behaviourKey string (e.g., "PlayerController").
      - The behaviour system uses behaviourKey to dispatch the correct behaviour at runtime.
      - The started flag is used to gate one-time initialization, and is reset during
        serialization to ensure predictable behaviour on reload.
    *****************************************************************************************/
    class BehaviourComponent : public GameComponent
    {
    public:
        std::string behaviourKey{}; ///< Behaviour/script identifier used for dispatch.
        bool started{ false };      ///< Runtime flag indicating whether behaviour has started.

        /*************************************************************************************
          \brief Serializes/deserializes the component state from a serializer.
          \param s  Serializer interface used to read/write component data.

          \details
          On deserialize/load, started is force-reset to false so behaviours re-run their
          initialization when a scene is loaded or reloaded. If the serializer contains
          a "behaviourKey" entry, it is read into behaviourKey.
        **************************************************************************************/
        void Serialize(ISerializer& s) override
        {
            started = false;
            if (s.HasKey("behaviourKey"))
            {
                StreamRead(s, "behaviourKey", behaviourKey);
            }
        }

        /*************************************************************************************
          \brief Creates a clone of this component using the engine's component pool.
          \return A ComponentHandle owning the cloned BehaviourComponent instance.

          \details
          The clone is allocated from ComponentPool<BehaviourComponent> and copy-constructed
          from the current component instance.
        **************************************************************************************/
        ComponentHandle Clone() const override
        {
            return ComponentPool<BehaviourComponent>::Create(*this);
        }
    };

}