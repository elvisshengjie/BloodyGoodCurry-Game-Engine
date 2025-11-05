/*********************************************************************************************
\file      EnemyTypeComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the EnemyTypeComponent class. This component
           stores and manages the type of an enemy entity (e.g., physical or ranged).

\details
           The EnemyTypeComponent provides a simple way to categorize enemies:
           - Supports two types: physical and ranged.
           - Supports serialization of the enemy type from a string.
           - Can be cloned for use across multiple enemy instances.

\note      This component does not implement any behavior; it is strictly a data container.

\copyright
           All content © 2025 DigiPen Institute of Technology Singapore.
           All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <string>
namespace Framework
{
    /*****************************************************************************************
    \class EnemyTypeComponent
    \brief
    Component responsible for storing and managing enemy type information.

    \details
    This component allows AI, combat, or rendering systems to query the type of enemy,
    enabling behavior differences based on type (e.g., melee vs. ranged attacks).
    *****************************************************************************************/
    class EnemyTypeComponent : public GameComponent 
    {
        public:
            /*****************************************************************************************
            \enum EnemyType
            \brief Represents the type of enemy.

            \details
            - `physical`: Melee or close-range enemies.
            - `ranged`: Enemies that attack from a distance.
            *****************************************************************************************/
            enum class EnemyType{physical, ranged};
            EnemyType Etype{EnemyType::physical};
            /*****************************************************************************************
            \brief
            Default constructor.
            *****************************************************************************************/
            EnemyTypeComponent() = default;
            /*****************************************************************************************
            \brief
            Constructor with specified enemy type.

            \param t
            EnemyType to assign to this component.
            *****************************************************************************************/
            EnemyTypeComponent(EnemyType t) : Etype(t){}
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Required override of GameComponent interface. Currently does nothing.
            *****************************************************************************************/
            void initialize() override {}
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
            Serializes or deserializes the enemy type.

            \param s
            Serializer instance.

            \details
            Reads the string `"type"` from the serializer and converts it to `EnemyType`.
            Defaults to `physical` if the string does not match `"ranged"`.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("type"))
                {
                    std::string typeStr;
                    StreamRead(s,"type",typeStr);
                    if(typeStr=="ranged" ||typeStr=="Ranged"){Etype = EnemyType::ranged;}
                    else Etype = EnemyType::physical;
                }
            }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new EnemyTypeComponent instance with the same type.

            \details
            Allows reuse of enemy type data across multiple enemy entities.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<EnemyTypeComponent>();
             copy->Etype = Etype;
             return copy;
            }
    };
}