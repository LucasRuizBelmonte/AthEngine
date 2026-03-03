#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_worldPos;
out vec2 v_uv;
out mat3 v_tbn;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));

    vec3 n = normalMatrix * a_normal;
    if (length(n) < 0.0001) n = vec3(0.0, 0.0, 1.0);
    n = normalize(n);

    vec3 t = normalMatrix * a_tangent;
    if (length(t) < 0.0001) t = vec3(1.0, 0.0, 0.0);
    t = normalize(t - n * dot(t, n));

    vec3 b = normalize(cross(n, t));

    v_worldPos = worldPos.xyz;
    v_uv = a_uv;
    v_tbn = mat3(t, b, n);
    gl_Position = u_proj * u_view * worldPos;
}
