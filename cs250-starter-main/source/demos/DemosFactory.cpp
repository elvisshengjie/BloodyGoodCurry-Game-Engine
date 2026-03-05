/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "DemosFactory.hpp"

#include "D01HelloQuad.hpp"
#include "util/Logger.hpp"


#include <algorithm>
#include <array>
#include <string>

namespace
{
    using namespace demos;

    using DemoCreateFunctionPtr = gsl::owner<IDemo*> (*)(void);

    struct DemoMetaData
    {
        Demos                 Type           = Demos::None;
        std::string_view      DisplayName    = "";
        std::string_view      ShortName      = "";
        DemoCreateFunctionPtr CreateFunction = []() -> gsl::owner<IDemo*> { return nullptr; };
    };

    constexpr std::array DemoInformation = {
        DemoMetaData{ Demos::HelloQuad, "Hello Quad", "hello", []() -> gsl::owner<IDemo*> { return new D01HelloQuad(); } },
    };

    constexpr std::string to_lower_and_remove_whitespace(std::string_view s)
    {
        std::string r;
        r.reserve(s.size());
        for (const char c : s)
        {
            if (std::isspace(c))
                continue;
            r.push_back(static_cast<char>(std::tolower(c)));
        }
        return r;
    };
}

namespace demos
{
    gsl::owner<IDemo*> create_demo(Demos the_demo)
    {
        if (const auto it = std::find_if(std::begin(DemoInformation), std::end(DemoInformation), [the_demo](const DemoMetaData& demo_info) { return demo_info.Type == the_demo; });
            it != DemoInformation.end())
        {
            try
            {
                return it->CreateFunction();
            }
            catch (const std::exception& e)
            {
                util::log_error("Failed to create demo {}\n{}\nloading default Hello Quad Demo", it->DisplayName, e.what());
                return new D01HelloQuad();
            }
        }
        util::log_debug("Unknown demo requested, loading default Hello Quad Demo");
        return new D01HelloQuad();
    }

    std::span<const DisplayNameType> get_supported_demos() noexcept
    {
        static const std::array<DisplayNameType, DemoInformation.size()> supported_demos = []()
        {
            std::array<DisplayNameType, DemoInformation.size()> values{};
            std::transform(
                std::begin(DemoInformation), std::end(DemoInformation), std::begin(values), [](const DemoMetaData& demo_info) { return DisplayNameType{ demo_info.DisplayName, demo_info.Type }; });
            return values;
        }();
        return std::span<const DisplayNameType>{ supported_demos };
    }

    std::string_view to_string(Demos the_demo) noexcept
    {
        if (const auto it = std::find_if(std::begin(DemoInformation), std::end(DemoInformation), [the_demo](const DemoMetaData& demo_info) { return demo_info.Type == the_demo; });
            it != DemoInformation.end())
        {
            return it->DisplayName;
        }
        return "Unknown Demo";
    }

    Demos to_enum(std::string_view the_demo) noexcept
    {
        const std::string demo_string = to_lower_and_remove_whitespace(the_demo);
        if (const auto it = std::find_if(
                std::begin(DemoInformation), std::end(DemoInformation),
                [&](const DemoMetaData& demo_info) { return demo_info.ShortName == demo_string || to_lower_and_remove_whitespace(demo_info.DisplayName) == demo_string; });
            it != DemoInformation.end())
        {
            return it->Type;
        }
        return Demos::None;
    }
}
