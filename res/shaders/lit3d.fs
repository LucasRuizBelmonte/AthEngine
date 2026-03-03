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

    vec3 lightDir = normalize(-u_lightDir);
    vec3 viewDir = normalize(u_viewPos - v_worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float NdotL = max(dot(n, lightDir), 0.0);
    float specExp = max(u_shininess, 1.0);
    float specTerm = pow(max(dot(n, halfDir), 0.0), specExp) * u_specularStrength;

    float specMask = 1.0;
    if (u_hasSpecularTex)
        specMask = texture(u_texSpecular, v_uv).r;

    vec3 ambient = 0.15 * baseColor;
    vec3 diffuse = NdotL * baseColor;
    vec3 specular = specTerm * specMask * vec3(1.0);

    vec3 emission = vec3(0.0);
    if (u_hasEmissionTex)
        emission = texture(u_texEmission, v_uv).rgb * u_emissionStrength;

    vec3 color = ambient + diffuse + specular + emission;
    FragColor = vec4(color, u_tint.a);
}
