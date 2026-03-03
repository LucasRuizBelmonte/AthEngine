#version 330 core

in vec3 v_worldPos;
in vec2 v_uv;
in mat3 v_tbn;

uniform sampler2D u_texBaseColor;
uniform sampler2D u_texSpecular;
uniform sampler2D u_texNormal;
uniform sampler2D u_texEmission;
uniform bool u_hasBaseColorTex;
uniform bool u_hasSpecularTex;
uniform bool u_hasNormalTex;
uniform bool u_hasEmissionTex;
uniform vec4 u_tint;
uniform float u_specularStrength;
uniform float u_shininess;
uniform float u_normalStrength;
uniform float u_emissionStrength;
uniform vec3 u_viewPos;
uniform vec3 u_lightDir;

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

void main()
{
    vec3 baseColor = u_tint.rgb;
    if (u_hasBaseColorTex)
    {
        baseColor *= texture(u_texBaseColor, v_uv).rgb;
    }

    vec3 n = normalize(v_tbn[2]);
    if (u_hasNormalTex)
    {
        vec3 nTex = texture(u_texNormal, v_uv).xyz * 2.0 - 1.0;
        nTex.xy *= u_normalStrength;
        nTex = normalize(nTex);
        n = normalize(v_tbn * nTex);
    }

    vec3 viewDir = normalize(u_viewPos - v_worldPos);

    float specMask = 1.0;
    if (u_hasSpecularTex)
        specMask = texture(u_texSpecular, v_uv).r;

    vec3 ambient = 0.05 * baseColor;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    int count = min(lightCount, MAX_LIGHTS);
    for (int i = 0; i < count; ++i)
    {
        Light l = lights[i];

        vec3 toLight = vec3(0.0);
        vec3 L = vec3(0.0);
        float attenuation = 1.0;

        if (l.type == 0)
        {
            L = normalize(-l.direction);
        }
        else
        {
            toLight = l.position - v_worldPos;
            float dist = length(toLight);
            if (dist > 0.0001)
                L = toLight / dist;

            if (l.range > 0.0001)
            {
                float falloff = clamp(1.0 - dist / l.range, 0.0, 1.0);
                attenuation *= falloff * falloff;
            }

            if (l.type == 2)
            {
                float cosTheta = dot(normalize(-l.direction), normalize(v_worldPos - l.position));
                float inner = clamp(l.innerCone, 0.0, 1.0);
                float outer = clamp(l.outerCone, 0.0, inner);
                float cone = smoothstep(outer, inner, cosTheta);
                attenuation *= cone;
            }
        }

        float NdotL = max(dot(n, L), 0.0);
        vec3 halfDir = normalize(L + viewDir);
        float specExp = max(u_shininess, 1.0);
        float specTerm = pow(max(dot(n, halfDir), 0.0), specExp) * u_specularStrength;

        vec3 lightColor = l.color * l.intensity * attenuation;
        diffuse += NdotL * baseColor * lightColor;
        specular += specTerm * specMask * lightColor;
    }

    if (count == 0)
    {
        vec3 lightDir = normalize(-u_lightDir);
        vec3 halfDir = normalize(lightDir + viewDir);
        float NdotL = max(dot(n, lightDir), 0.0);
        float specExp = max(u_shininess, 1.0);
        float specTerm = pow(max(dot(n, halfDir), 0.0), specExp) * u_specularStrength;
        diffuse += NdotL * baseColor;
        specular += specTerm * specMask * vec3(1.0);
        ambient = 0.15 * baseColor;
    }

    vec3 emission = vec3(0.0);
    if (u_hasEmissionTex)
        emission = texture(u_texEmission, v_uv).rgb * u_emissionStrength;

    vec3 color = ambient + diffuse + specular + emission;
    FragColor = vec4(color, u_tint.a);
}
