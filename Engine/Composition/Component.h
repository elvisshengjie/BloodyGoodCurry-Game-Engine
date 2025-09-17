
#pragma once
#include <cstdint>
#include <string>
#include "Common/ComponentTypeID.h"
#include "Common/Message.h"


//GameObjectComposition = collection of GameComponents.
//
//Each GameComponent = independent piece of behavior(Transform, Renderer, etc.).
//
//Components communicate via SendMessage and can query siblings via GetOwner()
namespace Framework
{
	class GameObjectComposition;

	class GameComponent {
	public: 
		virtual ~GameComponent() = default;

		//Lifecycle
		//called once when component is attached, Derived component override this to set themselves up
		virtual void initialize() {}
		//Used for communication between components and system
		virtual void SendMessage(Message& m) { (void)m; } // optional to override

		//Ownership access
		//Lets component get their owning GameObject( aka composition)
		// Example: PhysicsComponent might call GetOwner()->GetComponent<Transform>() to move it object
		GameObjectComposition* GetOwner() { return owner; }
		//read only
		GameObjectComposition const* GetOwner()const { return owner; }

		//return typeid
		// example :if (comp->GetTypeId() == ComponentTypeId::CT_Transform) { ... }
		ComponentTypeId GetTypeId() const { return type_id; }

	protected:
		//this are call when you add componenent to the game object
		//you dont want random code changing owner/type so it is protected
		// example to how add T*comp = new T();
		//						comp->set_owner(this);
		//                      comp_>set_type(ComponentTypeId::CT_Transform);
		void set_owner(GameObjectComposition* goc) { owner = goc; }
		void set_type(ComponentTypeId id) { type_id = id; }

		friend class GameObjectComposition;


	private:
		//back-pointer to the game object 
		GameObjectComposition* owner = nullptr;
		//store component type
		ComponentTypeId       type_id = ComponentTypeId::CT_None;

	};
}
