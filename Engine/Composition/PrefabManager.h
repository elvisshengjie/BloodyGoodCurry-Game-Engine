#pragma once
#include <unordered_map>
#include "Composition.h"
namespace Framework {

	extern std::unordered_map<std::string, GOC*> master_copies;

	void LoadPrefabs();

	inline GOC* ClonePrefab(const std::string& name) {
		auto it = master_copies.find(name);
		return (it == master_copies.end() || !it->second) ? nullptr : it->second->Clone();
	}
}