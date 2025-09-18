#pragma once
#include "Serialization.h"
#include <fstream>
#include <stack>
#include "../ThirdParty/json_dep/json.hpp"

namespace Framework
{
    using json = nlohmann::json;

    class JsonSerializer : public ISerializer
    {
    public:
        bool Open(const std::string& file) override;
        bool IsGood() override;

        bool EnterObject(const std::string& key) override;
        void ExitObject() override;
        bool HasKey(const std::string& key) const override;

        void ReadInt(const std::string& key, int& out) override;
        void ReadFloat(const std::string& key, float& out) override;
        void ReadString(const std::string& key, std::string& out) override;

    private:
        json root;
        std::stack<json*> objectStack; // keeps track of current object
    };
}
