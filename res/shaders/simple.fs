#version 330 core

in vec3 v_color;

uniform vec4 u_tint;

out vec4 FragColor;

void main()
{
    FragColor = vec4(v_color, 1.0) * u_tint;
}