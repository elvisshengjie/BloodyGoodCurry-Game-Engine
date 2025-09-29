#include "PrefabManager.h"
#include <iostream>
#include "Factory/Factory.h"
namespace Framework {
	std::unordered_map<std::string, Framework::GOC*> master_copies;

	void LoadPrefabs()
	{

		if (auto* c = FACTORY->Create("../../Data_Files/circle.json")) master_copies["Circle"] = c;
		if (auto* r = FACTORY->Create("../../Data_Files/rect.json"))   master_copies["Rect"] = r;

		if (auto* p = FACTORY->Create("../../Data_Files/player.json")) {
			master_copies["Player"] = p;
		}
		else {
			std::cerr << "[Prefab] Failed to create 'Player' from player.json\n";
		}
	}


}