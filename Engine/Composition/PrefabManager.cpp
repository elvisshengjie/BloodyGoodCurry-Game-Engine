#include "PrefabManager.h"
#include <iostream>
#include "Factory/Factory.h"
namespace Framework {
	std::unordered_map<std::string, Framework::GOC*> master_copies;

	void LoadPrefabs()
	{

		if (auto* c = FACTORY->CreateTemplate("../../Data_Files/circle.json")) master_copies["Circle"] = c;
		if (auto* r = FACTORY->CreateTemplate("../../Data_Files/rect.json"))   master_copies["Rect"] = r;

		if (auto* p = FACTORY->CreateTemplate("../../Data_Files/player.json")) {
			master_copies["Player"] = p;
		}
		else {
			std::cerr << "[Prefab] Failed to create 'Player' from player.json\n";
		}
	}

	void UnloadPrefabs() {
		for (auto& kv : master_copies) {
			delete kv.second;        // free raw GOC
			kv.second = nullptr;
		}
		master_copies.clear();
	}

}