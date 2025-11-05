/*********************************************************************************************
\file      EnemyComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the EnemyComponent class. This component
           acts as a simple data container for enemy entities without any active logic.

\details
           The EnemyComponent provides a minimal structure to tag or store enemy-related
           data. It inherits from GameComponent and implements required virtual functions
           with empty bodies. The component supports cloning for use in multiple enemy
           instances.

\note      This component does not manage any resources or perform any game logic by
           itself.

\copyright
           All content © 2025 DigiPen Institute of Technology Singapore.
           All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
namespace Framework
{
    /*****************************************************************************************
    \class EnemyComponent
    \brief
    A minimal data container for enemy entities.

    \details
    Implements the GameComponent interface but does not provide behavior. Suitable for
    tagging or holding basic enemy-related data. Supports cloning.
    *****************************************************************************************/
    class EnemyComponent : public GameComponent 
    {
        public:
          /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Empty implementation required by GameComponent interface.
            *****************************************************************************************/
            void initialize() override {}
            /*****************************************************************************************
            \brief
            Processes incoming messages.

            \param m
            Message to process.

            \details
            Empty implementation required by GameComponent interface.
            *****************************************************************************************/
            void SendMessage(Message& m) override { (void)m; }
             /*****************************************************************************************
            \brief
            Serializes or deserializes component data.

            \param s
            Serializer instance.

            \details
            Empty implementation required by GameComponent interface.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override { (void)s; }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new EnemyComponent instance.

            \details
            Allows the component to be duplicated for multiple entities.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {return std::make_unique<EnemyComponent>();}
    };
}