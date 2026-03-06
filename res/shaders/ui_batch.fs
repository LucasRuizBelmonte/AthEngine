#version 330 core

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texBaseColor;
uniform bool u_hasBaseColorTex;

out vec4 FragColor;

void main()
{
    vec4 baseColor = u_hasBaseColorTex ? texture(u_texBaseColor, v_uv) : vec4(1.0);
    FragColor = baseColor * v_color;
}

