/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IDemo.hpp"
#include <gsl/pointers>
#include <span>
#include <string_view>

namespace demos
{
    enum class Demos
    {
        None,
        HelloQuad,
    };

    gsl::owner<IDemo*> create_demo(Demos the_demo);

    struct DisplayNameType
    {
        std::string_view DisplayName;
        Demos            Type;
    };

    std::span<const DisplayNameType> get_supported_demos() noexcept;
    [[nodiscard]] std::string_view   to_string(Demos the_demo) noexcept;
    [[nodiscard]] Demos              to_enum(std::string_view the_demo_name) noexcept;
}
