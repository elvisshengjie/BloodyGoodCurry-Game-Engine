/*********************************************************************************************
\file      PlayerComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the PlayerComponent class. This component is a 
           basic player data container that currently does not hold any data or behavior but
           serves as a marker for player entities.

\details
           PlayerComponent provides minimal functionality:
           - Initialization prints a debug message.
           - Can be cloned for multiple player instances.
           - Supports basic serialization interface for future extensions.

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
    \class PlayerComponent
    \brief
    Basic component for player entities serving as a marker or placeholder.

    \details
    Currently, this component has no data or behavior aside from initialization logging.
    *****************************************************************************************/
    class PlayerComponent : public GameComponent 
    {
        public:
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Prints a debug message indicating that this object has a PlayerComponent.
            *****************************************************************************************/
            void initialize() override { std::cout << "This object has a PlayerComponent!\n"; }
            /*****************************************************************************************
            \brief
            Handles incoming messages.

            \param m
            Message to process.

            \details
            Required override of GameComponent interface. Currently does nothing.
            *****************************************************************************************/
            void SendMessage(Message& m) override { (void)m; }
            /*****************************************************************************************
            \brief
            Serializes or deserializes the component.

            \param s
            Serializer instance.

            \details
            Required override of GameComponent interface. Currently does nothing.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override { (void)s; }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new PlayerComponent instance.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {return std::make_unique<PlayerComponent>();}
    };
}