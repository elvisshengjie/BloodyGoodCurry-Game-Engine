#pragma once
/*********************************************************************************************
 \file      InspectorPanel.H
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Implements a basic ImGui inspector window for viewing and editing the currently
            selected GameObject's properties and common component data.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
namespace mygame
{
    /// Draws the ImGui inspector window for the currently selected game object.
    void DrawPropertiesEditor();
}