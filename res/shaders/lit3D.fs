#version 330 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;

uniform sampler2D u_texBaseColor;
uniform bool u_hasBaseColorTex;
uniform sampler2D u_texSpecular;
uniform bool u_hasSpecularTex;
uniform sampler2D u_texEmission;
uniform bool u_hasEmissionTex;

uniform vec4 u_tint;
uniform float u_specularStrength;
uniform float u_shininess;
uniform float u_emissionStrength;
uniform vec3 u_viewPos;

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

float ComputeSpotFactor(in Light l, vec3 fragmentDirFromLight)
{
    vec3 spotAxis = normalize(l.direction);
    float coneDot = dot(spotAxis, fragmentDirFromLight);

    float inner = max(l.innerCone, l.outerCone);
    float outer = min(l.innerCone, l.outerCone);

    if (abs(inner - outer) < 0.0001)
        return step(outer, coneDot);

    return smoothstep(outer, inner, coneDot);
}

void main()
{
    vec4 baseSample = u_hasBaseColorTex ? texture(u_texBaseColor, v_uv) : vec4(1.0);
    vec3 albedo = baseSample.rgb * u_tint.rgb;
    float alpha = baseSample.a * u_tint.a;

    float specularMask = u_hasSpecularTex ? texture(u_texSpecular, v_uv).r : 1.0;
    vec3 emission = vec3(0.0);
    if (u_hasEmissionTex)
        emission = texture(u_texEmission, v_uv).rgb * u_emissionStrength;

    vec3 n = normalize(v_worldNormal);
    vec3 viewDir = normalize(u_viewPos - v_worldPos);

    vec3 ambient = 0.05 * albedo;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    int count = clamp(lightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < count; ++i)
    {
        Light l = lights[i];

        vec3 L = vec3(0.0);
        float attenuation = 1.0;

        if (l.type == 0)
        {
            L = normalize(-l.direction);
        }
        else
        {
            vec3 toLight = l.position - v_worldPos;
            float dist = length(toLight);
            if (dist > 0.0001)
                L = toLight / dist;

            attenuation *= ComputeRangeAttenuation(dist, l.range);

            if (l.type == 2)
            {
                vec3 fragDirFromLight = (dist > 0.0001) ? normalize(v_worldPos - l.position) : vec3(0.0);
                attenuation *= ComputeSpotFactor(l, fragDirFromLight);
            }
        }

        float NdotL = max(dot(n, L), 0.0);
        if (NdotL <= 0.0 || attenuation <= 0.0)
            continue;

        vec3 halfDir = normalize(L + viewDir);
        float specExp = max(u_shininess, 1.0);
        float specTerm = pow(max(dot(n, halfDir), 0.0), specExp) * u_specularStrength * specularMask;

        vec3 lightColor = l.color * l.intensity * attenuation;
        diffuse += NdotL * albedo * lightColor;
        specular += specTerm * lightColor;
    }

    vec3 color = ambient + diffuse + specular + emission;
    FragColor = vec4(color, alpha);
}
