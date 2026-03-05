/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "D01HelloQuad.hpp"

#include "opengl/GL.hpp"
#include "window/Environment.hpp"
#include <array>
#include <imgui.h>
#include <span>

namespace
{
    struct float3
    {
        float x = 0, y = 0, z = 0;
    };

    struct color3
    {
        float r = 0, g = 0, b = 0;
    };

    struct texture2
    {
        float s = 0, t = 0;
    };

    struct Vertex
    {
        float3   position{};
        color3   color{};
        texture2 textureCoordinates{};
    };
}

namespace asset_paths
{
    constexpr auto HelloQuadVertexPath   = "D01HelloQuad/01_hello_quad.vert";
    constexpr auto HelloQuadFragmentPath = "D01HelloQuad/01_hello_quad.frag";
    const auto     ShaderName            = "hello quad shader";

    const auto PaintMeTexturePath = "D01HelloQuad/paint_me.png";
}

namespace demos
{
    D01HelloQuad::D01HelloQuad()
    {
        GL::ClearColor(0.392f, 0.584f, 0.929f, 1.0f);

        assetReloader.SetAndAutoReloadShader(shader, asset_paths::ShaderName, { asset_paths::HelloQuadVertexPath, asset_paths::HelloQuadFragmentPath });
        assetReloader.SetAndAutoReloadTexture(paintMeTexture, asset_paths::PaintMeTexturePath);

        const std::array vertices = {
            Vertex{ float3{ -0.75, 0.75f, 0 }, color3{ 1, 1, 0 }, texture2{ 0.0f, 1.0f } }, // top left
            Vertex{ float3{ -0.75, -0.75, 0 }, color3{ 1, 0, 0 }, texture2{ 0.0f, 0.0f } }, // bottom left
            Vertex{  float3{ 0.75, -0.75, 0 }, color3{ 0, 0, 1 }, texture2{ 1.0f, 0.0f } }, // bottom right
            Vertex{   float3{ 0.75, 0.75, 0 }, color3{ 0, 1, 0 }, texture2{ 1.0f, 1.0f } }, // top right
        };
        quadMesh.SetPrimitivePattern(opengl::Primitive::TriangleFan);
        const std::array     indices       = { 0u, 1u, 2u, 3u };
        const auto           vertices_span = std::span{ vertices };
        opengl::VertexBuffer buffer(vertices_span);
        opengl::IndexBuffer  index_buffer(indices);

        using namespace opengl;
        quadMesh.AddVertexBuffer(
            std::move(buffer), BufferLayout{
                                   Attribute::Float3, // position
                                   Attribute::Float3, // color
                                   Attribute::Float2  // texture coordinates
                               });
        quadMesh.SetIndexBuffer(std::move(index_buffer));
        assert(shader.IsValidWithVertexArrayObject(quadMesh.GetHandle()));
    }

    void D01HelloQuad::Update()
    {
        assetReloader.Update();
    }

    void D01HelloQuad::Draw() const
    {
        GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.Use();
        paintMeTexture.UseForSlot(0);
        shader.SendUniform("uTex2d", 0);
        quadMesh.Use();
        opengl::DrawIndexed(quadMesh);
    }

    void D01HelloQuad::ImGuiDraw()
    {
        ImGui::Text("%s", "Hello Quad!!!");
    }
}
