#include "SystemManager.h"



void Framework::SystemManager::IntializeAll()
{
	for (auto& sys : systems) {
		sys->Initialize();
	}
}

void Framework::SystemManager::UpdateAll(float dt)
{
	for (auto& sys : systems) {
		sys->Update(dt);
	}
}

void Framework::SystemManager::ShutdownAll()
{
	for (auto& sys : systems) {
		sys->Shutdown();
	}
	systems.clear();
}

Framework::SystemManager::~SystemManager()
{
	ShutdownAll();
}