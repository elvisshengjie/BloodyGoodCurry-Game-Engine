#pragma once
#include "Common/System.h"
namespace Framework {
	class InputSystem :public Framework::ISystem {
	public:
		void Initialize() override;

		void Update(float dt) override;
		
		void draw() override;
		
		void Shutdown() override;
		std::string GetName() override{ return "InputSystem"; }
	};
}