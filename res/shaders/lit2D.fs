#version 330 core

in vec2 v_uv;
in vec2 v_worldPosXY;

uniform sampler2D u_texBaseColor;
uniform bool u_hasBaseColorTex;
uniform vec4 u_tint;

#define MAX_LIGHTS 10

struct Light
{
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerCone;
    float outerCone;
};

uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

out vec4 FragColor;

float ComputeRangeAttenuation(float distanceToLight, float range)
{
    if (range <= 0.0001)
        return 0.0;
    return clamp(1.0 - (distanceToLight / range), 0.0, 1.0);
}

void main()
{
    vec4 baseSample = u_hasBaseColorTex ? texture(u_texBaseColor, v_uv) : vec4(1.0);
    vec3 albedo = baseSample.rgb * u_tint.rgb;
    float alpha = baseSample.a * u_tint.a;

    vec3 lit = 0.05 * albedo;

    int count = clamp(lightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < count; ++i)
    {
        Light l = lights[i];
        if (l.type != 1)
            continue;

        float dist = length(l.position.xy - v_worldPosXY);
        float attenuation = ComputeRangeAttenuation(dist, l.range);
        if (attenuation <= 0.0)
            continue;

        vec3 lightColor = l.color * l.intensity * attenuation;
        lit += albedo * lightColor;
    }

    FragColor = vec4(lit, alpha);
}
