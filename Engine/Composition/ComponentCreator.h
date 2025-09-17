#pragma once
#include <string>
#include "Component.h"
#include <memory>
//create a game Component but the details of creation to whoever inherit from me

namespace Framework {
	// an abstract base class for "Creator" of components
	class ComponentCreator {
	public:
		// explicit so that to prevent unintended implicit conversion 
		// ComponentCreator* c = new ComponentCreator(5);   // OK
		// ComponentCreator*c = 5                           // not allow
		explicit ComponentCreator(ComponentTypeId typeId) : TypeId(typeId) {}
		virtual ~ComponentCreator() = default;
		ComponentTypeId TypeId;
		// = 0 mean that it is a pure virtual function meaning the subclass have to implement it
		// and return the pointer to GameComponent
		virtual std::unique_ptr<GameComponent> Create() = 0;

	
	};

	//
	template<typename T>
	class ComponentCreatorType : public ComponentCreator {
	public:
		explicit ComponentCreatorType(ComponentTypeId typeId)
			: ComponentCreator(typeId) {
		}
		std::unique_ptr<GameComponent> Create()override { return std::make_unique<T>(); } // Use override to make usre if a derived class function is
		// not correctly overriding base class will result in compile time error
	};
}

//Register component macro
// factory API is: void AddComponentCreator(std::string, std::unique_ptr<ComponentCreator>);
//void AddComponentCreator(std::string name, std::unique_ptr<Framework::ComponentCreator> c);
#define RegisterComponent(type) \
  FACTORY->AddComponentCreator( \
      #type, std::make_unique<Framework::ComponentCreatorType<type>>( \
                 Framework::ComponentTypeId::CT_##type))