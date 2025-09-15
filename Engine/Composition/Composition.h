#pragma once

#include <vector>
#include <memory>
#include "Component.h"
#include "Common/Message.h"



namespace Framework {
	using GOCId = unsigned int;

	class GameObjectComposition {
	public: 
		friend class GameObjectFactory;

		// Broadcast a message to all component
		void SendMessage(Message& message);

		// Get first component matching type id (nullptr if none)
		GameComponent* GetComponent(ComponentTypeId typeId);
		//read only
		GameComponent const* GetComponent(ComponentTypeId typeId)const;


		template <typename T>
		T* GetComponentAs(ComponentTypeId typeId) {
			return static_cast<T*>(GetComponent(typeId));
		}

		template<typename T>
		T const* GetComponentAs(ComponentTypeId typeId)const {
			return static_cast<T*>(GetComponent(typeId))

		}

		//Lifecycle
		void initialize();
		void Destroy(); // mark for removal but actual deleted in factory

		//Add existing component
		void AddComponent(ComponentTypeId typeId, std::unique_ptr<GameComponent> comp);



		//construct add component of type T
		//A function parameter pack is a function parameter that accepts zero or more function arguments ...
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
		using UptrComp = std::unique_ptr<GameComponent>;
		std::vector<UptrComp> Components; //owned
		GOCId ObjectId = 0;

		GameObjectComposition() = default;
		~GameObjectComposition();



	};
}
