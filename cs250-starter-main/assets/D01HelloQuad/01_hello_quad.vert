#version 300 es
precision highp float;

/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec3 aVertexPosition;
layout(location = 1) in vec3 aVertexColor;
layout(location = 2) in vec2 aVertexTextureCoordinates;

out vec3 vColor;
out vec2 vTextureCoordinates;

void main()
{
    gl_Position         = vec4(aVertexPosition, 1.0);
    vColor              = aVertexColor;
    vTextureCoordinates = aVertexTextureCoordinates;
}
