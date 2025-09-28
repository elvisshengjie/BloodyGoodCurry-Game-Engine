#pragma once
#pragma once

// Minimal, prefab-agnostic spawn UI for ImGui.
// Usage:
//   1) #include "Debug/Spawn.h" in Game.cpp
//   2) Call mygame::DrawSpawnPanel() each frame (between BeginFrame/EndFrame)

namespace mygame {

	struct SpawnSettings {
		// always available (normalized 0..1 if your renderer uses NDC-like coords)
		float x{ 0.5f }, y{ 0.5f };
		float rot{ 0.0f };         // radians

		// rect (RenderComponent)
		float w{ 0.12f }, h{ 0.08f };

		// circle (CircleRenderComponent)
		float radius{ 0.08f };

		// tint for anything renderable
		float rgba[4]{ 1.f, 1.f, 1.f, 1.f };

		// batch
		int   count{ 1 };
		float stepX{ 0.05f }, stepY{ 0.0f };
	};

	// Draws the "Spawn" ImGui panel and performs spawning when user clicks.
	void DrawSpawnPanel();

} // namespace mygame
