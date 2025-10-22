#pragma once
#include "Common/System.h"
#include "Messaging_System/Messager_Bus.hpp"

#include <array>
#include <memory>
namespace Framework {
	class AudioSystem :public Framework::ISystem {
	public:
		AudioSystem();

		void Initialize() override;

		void Update(float dt) override;


		void Shutdown() override;

		std::string GetName() override{ return "AudioSystem"; }



	private:

	};
}