#pragma once
#include <string>
#include "Message.h"


namespace Framework {
	class ISystem {
	public:
		virtual ~ISystem() = default;
		virtual void Initialize() {}
		virtual void SendMessage(Message* m) { (void)m; } // tell complier i know parameter is unused dont warn me
		virtual void Update(float dt) = 0; // pure virtual function all derived class must implement it
		virtual std::string GetName() = 0;
	};
}