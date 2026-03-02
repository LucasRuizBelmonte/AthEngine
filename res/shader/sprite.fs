#version 330 core

in vec2 v_uv;

uniform sampler2D u_tex0;
uniform vec4 u_tint;

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(u_tex0, v_uv);
    FragColor = texColor * u_tint;
}