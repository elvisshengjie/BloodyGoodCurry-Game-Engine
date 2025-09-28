#include "PrefabManager.h"

#include "Factory/Factory.h"
namespace Framework {
	std::unordered_map<std::string, Framework::GOC*> master_copies;

	void LoadPrefabs()
	{

		master_copies["Circle"] = FACTORY->Create("../../Data_Files/circle.json");
		master_copies["Rect"] = FACTORY->Create("../../Data_Files/rect.json");
	}


}