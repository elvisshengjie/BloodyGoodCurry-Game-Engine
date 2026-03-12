/*********************************************************************************************
 \file      ParticleVfxPresetPersistence.cpp
 \par       SofaSpuds
 \author
 \brief     Implements persistence helpers for gameplay particle and VFX presets.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "ParticleVfxPresetPersistence.h"

#include "ParticlePresets.hpp"
#include "VfxPresets.hpp"

#include "Core/PathUtils.h"

#include <json.hpp>
#include <fstream>
#include <iostream>
#include <system_error>

namespace mygame
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        EnemyDeathParticlePreset,
        count,
        speedMin,
        speedMax,
        lifeMin,
        lifeMax,
        radiusMin,
        radiusMax,
        upwardVelocityBias,
        red,
        green,
        blue,
        greenJitterMin,
        greenJitterMax,
        startAlpha,
        endAlpha,
        endRadiusScale)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        RunParticlePreset,
        count,
        speedMin,
        speedMax,
        lifeMin,
        lifeMax,
        sizeMin,
        sizeMax,
        jitterMin,
        jitterMax,
        riseMin,
        riseMax,
        offsetX,
        offsetY,
        endSizeScale,
        red,
        green,
        blue,
        startAlpha,
        endAlpha)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        HitImpactBurstPreset,
        count,
        speedMin,
        speedMax,
        lifeMin,
        lifeMax,
        radiusMin,
        radiusMax,
        offsetMin,
        offsetMax,
        endRadiusScale,
        red,
        green,
        blue,
        startAlpha,
        endAlpha)

    namespace
    {
        constexpr const char* kPresetFileName = "particle_vfx_presets.json";
        constexpr int kPresetFileVersion = 1;
    }

    std::filesystem::path GetParticleVfxPresetFilePath()
    {
        return Framework::ResolveDataPath(kPresetFileName);
    }

    bool LoadParticleVfxPresetsFromDisk()
    {
        const auto path = GetParticleVfxPresetFilePath();
        std::ifstream in(path);
        if (!in.is_open())
            return false;

        nlohmann::json root = nlohmann::json::parse(in, nullptr, false);
        if (root.is_discarded() || !root.is_object())
        {
            std::cerr << "[ParticleVfxPresetPersistence] Failed to parse preset file: "
                      << path << "\n";
            return false;
        }

        if (root.contains("enemyDeath") && root["enemyDeath"].is_object())
            GetEnemyDeathParticlePreset() = root["enemyDeath"].get<EnemyDeathParticlePreset>();

        if (root.contains("runTrail") && root["runTrail"].is_object())
            GetRunParticlePreset() = root["runTrail"].get<RunParticlePreset>();

        if (root.contains("hitImpact") && root["hitImpact"].is_object())
            GetHitImpactBurstPreset() = root["hitImpact"].get<HitImpactBurstPreset>();

        return true;
    }

    bool SaveParticleVfxPresetsToDisk()
    {
        const auto path = GetParticleVfxPresetFilePath();
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            std::cerr << "[ParticleVfxPresetPersistence] Failed to create preset directory: "
                      << path.parent_path() << "\n";
            return false;
        }

        nlohmann::json root = nlohmann::json::object();
        root["version"] = kPresetFileVersion;
        root["enemyDeath"] = GetEnemyDeathParticlePreset();
        root["runTrail"] = GetRunParticlePreset();
        root["hitImpact"] = GetHitImpactBurstPreset();

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            std::cerr << "[ParticleVfxPresetPersistence] Failed to open preset file for write: "
                      << path << "\n";
            return false;
        }

        out << root.dump(2);
        return out.good();
    }
}
