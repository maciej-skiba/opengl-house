#version 330 core
layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec3 WorldPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldSpacePos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldSpacePos;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    WorldPos = worldSpacePos.xyz;
    TexCoords = aTexCoord;
}