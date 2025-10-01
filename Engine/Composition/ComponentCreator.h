/*********************************************************************************************
 \file      ComponentCreator.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the ComponentCreator system, which provides an abstract interface
            and templated implementation for dynamically creating game components at
            runtime. Supports registration via the RegisterComponent macro for use in
            the GameObjectFactory’s component registry.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include <string>
#include "Component.h"
#include <memory>

//create a game Component but the details of creation to whoever inherit from me

namespace Framework {
    /*****************************************************************************************
      \class ComponentCreator
      \brief Abstract base class for component creators.

      Provides a uniform interface for dynamically creating game components without
      knowing their concrete type. Each creator is associated with a ComponentTypeId
      for lookup and registration in the factory.
    *****************************************************************************************/
    class ComponentCreator {
    public:
        // explicit so that to prevent unintended implicit conversion 
        // ComponentCreator* c = new ComponentCreator(5);   // OK
        // ComponentCreator*c = 5                           // not allow
        /*************************************************************************************
          \brief Constructs a ComponentCreator with the given type ID.
          \param typeId  The ComponentTypeId that this creator is responsible for.
        *************************************************************************************/
        explicit ComponentCreator(ComponentTypeId typeId) : TypeId(typeId) {}

        /*************************************************************************************
          \brief Virtual destructor for safe polymorphic deletion.
        *************************************************************************************/
        virtual ~ComponentCreator() = default;

        ComponentTypeId TypeId; ///< The type identifier for the component created by this creator

        // = 0 mean that it is a pure virtual function meaning the subclass have to implement it
        // and return the pointer to GameComponent
        /*************************************************************************************
          \brief Creates a new instance of the component.
          \return Raw pointer to the newly created GameComponent.
          \note   Must be overridden by derived creators.
        *************************************************************************************/
        virtual GameComponent* Create() = 0;
    };

    /*****************************************************************************************
      \class ComponentCreatorType
      \brief Templated concrete creator for a specific component type.

      Implements the Create() function by instantiating objects of type T. Used in
      conjunction with the RegisterComponent macro to simplify registration.
    *****************************************************************************************/
    template<typename T>
    class ComponentCreatorType : public ComponentCreator {
    public:
        /*************************************************************************************
          \brief Constructs a ComponentCreatorType for the given component type ID.
          \param typeId  The ComponentTypeId associated with the component type T.
        *************************************************************************************/
        explicit ComponentCreatorType(ComponentTypeId typeId)
            : ComponentCreator(typeId) {
        }

        /*************************************************************************************
          \brief Creates a new instance of the component of type T.
          \return Raw pointer to a new T instance.
          \note   Uses override to enforce correct function signature at compile time.
        *************************************************************************************/
        GameComponent* Create() override { return new T(); } // Use override to make sure if a derived class function is
        // not correctly overriding base class will result in compile time error
    };
}

//Register component macro
// factory API is: void AddComponentCreator(std::string, std::unique_ptr<ComponentCreator>);
//void AddComponentCreator(std::string name, std::unique_ptr<Framework::ComponentCreator> c);
/*****************************************************************************************
  \def RegisterComponent
  \brief Registers a component type with the global factory.

  This macro creates a ComponentCreatorType for the given type and associates it
  with its ComponentTypeId. The creator is added to the factory’s registry under
  the stringified type name.

  Example:
  \code
      RegisterComponent(TransformComponent);
  \endcode
*****************************************************************************************/
#define RegisterComponent(type) \
  FACTORY->AddComponentCreator( \
      #type, new Framework::ComponentCreatorType<type>( \
                 Framework::ComponentTypeId::CT_##type))
