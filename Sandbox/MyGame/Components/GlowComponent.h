/*********************************************************************************************
 \file      GlowComponent.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 80%
            yimo.kong ( yimo.kong@digipen.edu) - Author, 20%
 \brief     Declares the GlowComponent class, a procedural glow renderer that supports
            freehand point strokes, configurable color/opacity, and radial falloff.
            Supports JSON serialization for data-driven initialization and cloning for
            prefab instancing.
 \details   The component stores glow parameters in local space so authoring tools and
            gameplay code can attach painted glow strokes to any transform without
            requiring sprite-sheet assets.

 \copyright
            All content 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include <glm/vec2.hpp>
#include <vector>

namespace Framework {
    /*****************************************************************************************
      \class GlowComponent
      \brief A rendering component specialized for procedural glow blobs and strokes.

      Stores color/opacity, brightness, inner/outer radius, falloff exponent, and a list
      of local-space points that define a freehand stroke. Each point renders a radial
      glow without needing a texture.
    *****************************************************************************************/
    class GlowComponent : public GameComponent {
    public:
        float r{ 1.f }, g{ 0.8f }, b{ 0.3f }; ///< Base glow color.
        float opacity{ 1.f }; ///< Alpha applied to the procedural glow.
        float brightness{ 1.f }; ///< Intensity multiplier applied by the renderer.
        float innerRadius{ 0.05f }; ///< Radius of the fully bright inner region.
        float outerRadius{ 0.2f }; ///< Outer falloff radius for each glow point.
        float falloffExponent{ 1.0f }; ///< Controls how quickly the glow fades to zero.
        bool  visible{ true }; ///< Toggles rendering without discarding stored stroke data.

        std::vector<glm::vec2> points{}; ///< Local-space stroke points (relative to owner transform).

        /*************************************************************************************
          \brief  Initialize the component.
          \details No runtime setup is currently required for procedural glow data.
        *************************************************************************************/
        void initialize() override {}

        /*************************************************************************************
          \brief  Receive engine messages.
          \param  m Message payload (unused).
        *************************************************************************************/
        void SendMessage(Message& m) override { (void)m; }

        /*************************************************************************************
          \brief  Serialize glow parameters and local-space stroke points.
          \param  s Serializer used for load/save.
          \details Accepts both "opacity" and legacy "a" alpha keys for compatibility.
        *************************************************************************************/
        void Serialize(ISerializer& s) override {
            if (s.HasKey("r")) StreamRead(s, "r", r);
            if (s.HasKey("g")) StreamRead(s, "g", g);
            if (s.HasKey("b")) StreamRead(s, "b", b);
            if (s.HasKey("opacity")) StreamRead(s, "opacity", opacity);
            if (s.HasKey("a")) StreamRead(s, "a", opacity);
            if (s.HasKey("brightness")) StreamRead(s, "brightness", brightness);
            if (s.HasKey("inner_radius")) StreamRead(s, "inner_radius", innerRadius);
            if (s.HasKey("outer_radius")) StreamRead(s, "outer_radius", outerRadius);
            if (s.HasKey("falloff_exponent")) StreamRead(s, "falloff_exponent", falloffExponent);
            if (s.HasKey("visible")) StreamRead(s, "visible", visible);

            points.clear();
            if (s.EnterArray("points"))
            {
                const size_t count = s.ArraySize();
                points.reserve(count);
                for (size_t i = 0; i < count; ++i)
                {
                    if (!s.EnterIndex(i))
                        continue;
                    float x = 0.f;
                    float y = 0.f;
                    if (s.HasKey("x")) StreamRead(s, "x", x);
                    if (s.HasKey("y")) StreamRead(s, "y", y);
                    points.emplace_back(x, y);
                    s.ExitObject();
                }
                s.ExitArray();
            }
        }

        /*************************************************************************************
          \brief  Clone the component for prefab instancing.
          \return A deep-copied GlowComponent containing the same stroke and render data.
        *************************************************************************************/
        ComponentHandle Clone() const override {
            auto copy = ComponentPool<GlowComponent>::CreateTyped();
            copy->r = r;
            copy->g = g;
            copy->b = b;
            copy->opacity = opacity;
            copy->brightness = brightness;
            copy->innerRadius = innerRadius;
            copy->outerRadius = outerRadius;
            copy->falloffExponent = falloffExponent;
            copy->visible = visible;
            copy->points = points;
            return copy;
        }
    };
}
