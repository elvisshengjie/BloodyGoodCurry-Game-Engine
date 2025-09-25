#pragma once
#include "Serialization.h"
#include <fstream>
#include <stack>
#pragma warning(push)           // Save current warning state
#pragma warning(disable:26819)  // Disable 'Unannotated fallthrough' warning
#include "../ThirdParty/json_dep/json.hpp"
#pragma warning(pop)             // Restore warnings after this point

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

        bool EnterArray(const std::string& key) override;
        void ExitArray() override;
        size_t ArraySize() const override;
        bool EnterIndex(size_t i) override;

    private:
        json root;
        std::stack<json*> objectStack; // keeps track of current object
    };
}
