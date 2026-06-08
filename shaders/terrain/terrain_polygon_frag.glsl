#version 330 core

out vec4 FragColor;

uniform vec3 polygonModeColor;

void main()
{
    FragColor = vec4(polygonModeColor, 1.0);
}