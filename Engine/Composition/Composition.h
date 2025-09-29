#pragma once

#include <vector>
#include <memory>
#include "Component.h"
#include "Common/Message.h"
#include <string>

namespace Framework {
	using GOCId = unsigned int; //Alias for a game ID

	class GameObjectComposition {   //Entity/"composition" that own component
	public: 
		friend class GameObjectFactory; //Grant factory access

		//Set and get name
		void SetObjectName(const std::string& name) { ObjectName = name; }
		const std::string& GetObjectName() const { return ObjectName; }

		// Broadcast a message to all component
		void SendMessage(Message& message);

		// Get first component matching type id (nullptr if none)
		GameComponent* GetComponent(ComponentTypeId typeId);
		//read only overload preserve when GOC itself is const
		GameComponent const* GetComponent(ComponentTypeId typeId)const;

		//Clone GameObject
		GameObjectComposition* Clone() const;

		//Find the first component with the give type ID and return it
		template <typename T>
		T* GetComponentAs(ComponentTypeId typeId) {
			return static_cast<T*>(GetComponent(typeId));
		}

		template<typename T>
		T const* GetComponentAs(ComponentTypeId typeId)const {
			return static_cast<T const*>(GetComponent(typeId));

		}

		///Type safe way of accessing components.
		template<typename T>
		T* GetComponentType(ComponentTypeId typeId);
		// const overload
		template <typename T>
		T const* GetComponentType(ComponentTypeId typeId) const;

		//Lifecycle
		void initialize(); //call initialize() on all component
		void Destroy(); // mark for removal but actual deleted in factory

		//Add existing component
		void AddComponent(ComponentTypeId typeId, std::unique_ptr<GameComponent> comp);



		//construct add component of type T
		//A function parameter pack is a function parameter that accepts zero or more function arguments ...
		//build the component
		//Wires its owner back-pointer and type id
		//transfer ownership to the composition
		//returns a handy non-owning raw pointer to the new component
		// example : auto* t = goc.EmplaceComponent<TransformComponent>(ComponentTypeId::CT_Transform, 0.0f, 0.0f);
		//         later   t->SetPosition(15.0f,25.0f);
		template <typename T, typename... Args>
		T* EmplaceComponent(ComponentTypeId typeId, Args&&... args) {
			auto up = std::make_unique<T>(std::forward<Args>(args)...);
			T* raw = up.get();
			raw->set_owner(this);
			raw->set_type(typeId);
			Components.emplace_back(std::move(up));
			return raw;
		}

		GOCId GetId() const { return ObjectId;  }

	private:
		// use unique pointer as The composition exclusively owns its components when the GameObjectComposition
		//is destroy every unique_ptr is delete its component
		//unique pointer are move-only so a component instance cant be owned by 2 game object
		using UptrComp = std::unique_ptr<GameComponent>;
		std::vector<UptrComp> Components; //owned 
		GOCId ObjectId = 0;
		std::string ObjectName;

		GameObjectComposition() = default;
		~GameObjectComposition();



	};
	using GOC  = GameObjectComposition;
	template<typename T>
	inline T* GameObjectComposition::GetComponentType(ComponentTypeId typeId) {
		return static_cast<T*>(GetComponent(typeId));
	}

	template<typename T>
	inline T const* GameObjectComposition::GetComponentType(ComponentTypeId typeId) const {
		return static_cast<T const*>(GetComponent(typeId));
	}

	#define HAS(obj, Type) ((obj)->GetComponentType<Type>(Framework::ComponentTypeId::CT_##Type))
	// how to use it
	// usage:
	// auto* t = HAS(obj, Transform);
}
