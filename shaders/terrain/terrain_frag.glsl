#version 330 core

out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 camPos;

// Material without textures
uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// Directional light
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

// Fog
uniform vec3 fogColor;
uniform float fogDensity;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    float clampedRoughness = clamp(roughness, 0.04, 1.0);
    float clampedMetallic = clamp(metallic, 0.0, 1.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, clampedMetallic);

    vec3 Lo = vec3(0.0);

    // Directional light
    vec3 L = normalize(-dirLightDirection);
    vec3 H = normalize(V + L);

    vec3 radiance = dirLightColor;

    float NDF = DistributionGGX(N, H, clampedRoughness);
    float G = GeometrySmith(N, V, L, clampedRoughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    // Metals do not have diffuse reflection
    kD *= 1.0 - clampedMetallic;

    vec3 numerator = NDF * G * F;

    float denominator =
        4.0 *
        max(dot(N, V), 0.0) *
        max(dot(N, L), 0.0)
        + 0.0001;

    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    // Simple ambient
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    // Fog
    float dist = length(camPos - WorldPos);
    float fogFactor = exp(-fogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    color = mix(fogColor, color, fogFactor);

    FragColor = vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float numerator = a2;

    float denominator = NdotH2 * (a2 - 1.0) + 1.0;
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

    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(
        clamp(1.0 - cosTheta, 0.0, 1.0),
        5.0
    );
}