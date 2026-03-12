/*********************************************************************************************
 \file      ParticleVfxPresetPersistence.h
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Declares persistence helpers for gameplay particle and VFX presets.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once


#include <filesystem>

namespace mygame
{
    std::filesystem::path GetParticleVfxPresetFilePath();
    bool LoadParticleVfxPresetsFromDisk();
    bool SaveParticleVfxPresetsToDisk();
}
