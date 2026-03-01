/*********************************************************************************************
 \file      BehaviourFCT.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares BehaviourFCT, a small function-table (FCT) that stores callbacks for a
            behaviour's lifecycle: Init, Update, and End.

 \details
            BehaviourFCT provides a lightweight, allocation-free way to bind behaviour logic
            to a game object using plain function pointers (instead of virtual dispatch).
            Systems can store/lookup a BehaviourFCT by key and then invoke:
            - Init  : one-time setup for the object
            - Update: per-frame logic with dt
            - End   : cleanup/exit logic when the behaviour stops or the object is destroyed

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

namespace Framework {
    class GameObjectComposition;

    /*****************************************************************************************
      \struct BehaviourFCT
      \brief Function-pointer table representing a behaviour's lifecycle callbacks.

      \details
      The behaviour system can register a BehaviourFCT per behaviourKey and then call the
      stored functions for a GameObjectComposition. Any callback may be left as nullptr,
      and callers should null-check before invoking.
    *****************************************************************************************/
    struct BehaviourFCT
    {
        /*************************************************************************************
          \typedef InitFn
          \brief Signature for behaviour initialization callbacks.
          \param obj  The object composition owning this behaviour.
        **************************************************************************************/
        using InitFn = void(*)(GameObjectComposition*);

        /*************************************************************************************
          \typedef UpdateFn
          \brief Signature for per-frame update callbacks.
          \param obj  The object composition owning this behaviour.
          \param dt   Delta time in seconds for this update tick.
        **************************************************************************************/
        using UpdateFn = void(*)(GameObjectComposition*, float);

        /*************************************************************************************
          \typedef EndFn
          \brief Signature for behaviour termination/cleanup callbacks.
          \param obj  The object composition owning this behaviour.
        **************************************************************************************/
        using EndFn = void(*)(GameObjectComposition*);

        InitFn Init{ nullptr };       ///< Optional initialization callback (called once).
        UpdateFn Update{ nullptr };   ///< Optional per-frame update callback.
        EndFn End{ nullptr };         ///< Optional cleanup/exit callback.
    };
}