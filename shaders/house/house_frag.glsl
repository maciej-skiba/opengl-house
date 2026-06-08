#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

struct Material
{
    sampler2D texture_albedo1;
    sampler2D texture_metallicRoughness1;
};

uniform Material material;

uniform vec3 lightPosition;
uniform vec3 lightColor;

uniform vec3 camPos;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec3 albedo = texture(material.texture_albedo1, TexCoords).rgb;

    // glTF metallic-roughness convention:
    // G = roughness
    // B = metallic
    vec3 mr = texture(material.texture_metallicRoughness1, TexCoords).rgb;
    float roughness = mr.g;
    float metallic  = mr.b;

    float ao = 1.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    vec3 L = normalize(lightPosition - WorldPos);
    vec3 H = normalize(V + L);

    float distance = length(lightPosition - WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;

    float denominator =
        4.0 *
        max(dot(N, V), 0.0) *
        max(dot(N, L), 0.0)
        + 0.0001;

    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float numerator = a2;

    float denominator =
        NdotH2 * (a2 - 1.0) + 1.0;

    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(
        clamp(1.0 - cosTheta, 0.0, 1.0),
        5.0
    );
}