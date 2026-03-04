#version 330 core

in vec2 v_uv;

uniform sampler2D u_texBaseColor;
uniform bool u_hasBaseColorTex;
uniform vec4 u_tint;

out vec4 FragColor;

void main()
{
    vec4 baseColor = u_hasBaseColorTex ? texture(u_texBaseColor, v_uv) : vec4(1.0);
    FragColor = baseColor * u_tint;
}
