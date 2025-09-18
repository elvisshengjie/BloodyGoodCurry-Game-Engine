#pragma once
#include "Serialization.h"
#include <fstream>
#include "../ThirdParty/json_dep/json.hpp"


namespace Framework
{
	using json = nlohmann::json;
	class JsonSerializer : public ISerializer
	{
	public:
		std::ifstream stream;
		virtual bool Open(const std::string& file);
		virtual bool IsGood();
		virtual void ReadInt(int& i);
		virtual void ReadFloat(float& f);
		virtual void ReadString(std::string& str);

	};
	
}