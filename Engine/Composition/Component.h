
#pragma once
#include "Common/ComponentTypeID.h"
#include "Common/Message.h"


namespace Framework
{
	//Forwaed declaration of GOC class
	class GameObject;
	using GOC = GameObject;

	class GameComponent
	{
	public: 
		friend class GameObject;
		
		//Signal that component is now active in the game world
		virtual void initialize() {}

		//GameComonent receives all messages send their owning composition
		virtual void SendMesage(Message*) {};


		
		/*virtual void Seerialize(ISerializer& str) {}*/

		GOC* GetOwner() { return Base; }
		
		ComponentTypeId TypeId{ ComponentTypeId::CT_None };

	protected:
		//Destroy the component
		virtual ~GameComponent() {}
	private:
		GOC* Base{nullptr};



	};
}