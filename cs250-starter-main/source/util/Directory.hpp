/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <filesystem>

namespace util
{
	std::filesystem::path get_writable_app_directory() noexcept;
}
