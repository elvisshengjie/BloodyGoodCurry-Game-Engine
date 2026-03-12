/*********************************************************************************************
 \file      Blackboard.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%
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
	/*****************************************************************************************
	  \class BlackBoard
	  \brief Lightweight type-erased key-value store for sharing AI state between nodes.
	  \details
	  Values are stored as std::any and retrieved by exact type. If a key is missing or
	  the stored type does not match the requested type, the fallback value is returned
	  instead of throwing. Intended for simple flags and scalars (bool, float, int) passed
	  between behavior tree nodes without coupling them directly to each other.
	*****************************************************************************************/
	class BlackBoard
	{
	public:
		/*************************************************************************************
		  \brief Stores or overwrites a value under the given key.
		  \tparam T    Type of the value to store.
		  \param  key  String key to associate the value with.
		  \param  value Value to store; moved into the internal map.
		*************************************************************************************/
		template <typename T>
		void Set(const std::string& key, T value) { data[key] = std::move(value); }
		/*************************************************************************************
		\brief Retrieves a value by key, returning a fallback if missing or type-mismatched.
		\tparam T        Expected type of the stored value.
		\param  key      String key to look up.
		\param  fallback Value returned if the key does not exist or the cast fails.
		\return The stored value cast to T, or fallback on failure.
	  *************************************************************************************/
		template <typename T>
		T Get(const std::string& key, T fallback = T{})
		{
			auto it = data.find(key);
			if (it == data.end()) return fallback;
			const T* ptr = std::any_cast<T> (&it->second);
			return ptr ? *ptr : fallback;
		}
		/*************************************************************************************
		  \brief Returns true if a value is stored under the given key.
		  \param key String key to check.
		  \return True if the key exists in the blackboard.
		*************************************************************************************/
		bool Has(const std::string& key) const { return data.count(key) > 0; }
		/*************************************************************************************
		 \brief Removes the entry associated with the given key, if present.
		 \param key String key to erase.
	   *************************************************************************************/
		void Clear(const std::string& key) { data.erase(key); }

	private:
		std::unordered_map<std::string, std::any> data;  ///< Internal key-value store; values are type-erased via std::any.
	};
}
