#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_worldPos;
out vec3 v_worldNormal;
out vec2 v_uv;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));

    vec3 worldNormal = normalMatrix * a_normal;
    if (length(worldNormal) < 0.0001) worldNormal = vec3(0.0, 0.0, 1.0);

    v_worldPos = worldPos.xyz;
    v_worldNormal = normalize(worldNormal);
    v_uv = a_uv;
    gl_Position = u_proj * u_view * worldPos;
}
