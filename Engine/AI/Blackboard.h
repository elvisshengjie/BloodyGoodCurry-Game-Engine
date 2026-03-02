/*********************************************************************************************
 \file      Blackboard.h
 \par       SofaSpuds
 \author
 \brief     Defines a lightweight key-value blackboard for AI state sharing.
 \details   Stores arbitrary values by string key using std::any so behavior logic can
            exchange simple state without tight coupling between nodes and systems.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once
#include <any>
#include <string>
#include <unordered_map>
namespace Framework
{
	class BlackBoard
	{
	public:
		template <typename T>
		void Set(const std::string& key, T value) { data[key] = std::move(value); }

		template <typename T>
		T Get(const std::string& key, T fallback = T{})
		{
			auto it = data.find(key);
			if (it == data.end()) return fallback;
			const T* ptr = std::any_cast<T >> (&it->second);
			return ptr ? *ptr : fallback;
		}

		bool Has(const std::string& key) const { return data.count(key) > 0; }

		void Clear(const std::string& key) { data.erase(key); }

	private:
		std::unordered_map<std::string, std::any> data;
	};
}
