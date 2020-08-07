#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "base.cmf"

layout(binding = 16) uniform sampler2D texAlbedo;
layout(binding = 17) uniform sampler2D texNormal;
layout(binding = 18) uniform sampler2D texDisplacement;
layout(binding = 19) uniform sampler2D texRoughness;
layout(binding = 20) uniform sampler2D texMetalness;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

//http://www.thetenthplanet.de/archives/1180
mat3 CalculateTBN(vec3 pos, vec3 N, vec2 uv) {
    vec3 dxPos = dFdxFine(pos);
    vec3 dyPos = dFdyFine(pos);
    vec2 dxUV = dFdxFine(uv);
    vec2 dyUV = dFdyFine(uv);

    vec3 xPerp = cross(N, dxPos);
    vec3 yPerp = cross(dyPos, N);
    
    vec3 T = dxUV.x * yPerp + dyUV.x * xPerp;
    vec3 B = dxUV.y * yPerp + dyUV.y * xPerp;
    
    //T = normalize(T - N * dot(T, N));
    //B = normalize(cross(N, T));
    B = normalize(B - N * dot(B, N));
    T = -normalize(cross(N, B));
    return mat3( T, B , N );
}

vec3 TransposeNormal(vec3 localNormal, mat3 TBN) {
    return normalize(TBN * localNormal);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = M_PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

void main() {
    vec4 albedo = texture(texAlbedo, inTexCoord);
    vec3 normal = texture(texNormal, inTexCoord).rgb;
    float displacement = texture(texDisplacement, inTexCoord).r;
    float roughness = texture(texRoughness, inTexCoord).r;
    vec3 metalness = texture(texMetalness, inTexCoord).rrr;

    vec3 worldNormal = normalize(inWorldNormal);

    mat3 TBN = CalculateTBN(inWorldPos, worldNormal, inTexCoord);
    vec3 N = TransposeNormal(normal, TBN);
    vec3 V = normalize(commonBuffer.viewPos - inWorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo.rgb, metalness);

    // reflectance equation
    vec3 Lo = vec3(0.0f);
    vec3 ambientColor = vec3(0.0f);
    for(int i = 0; i < 1; ++i) 
    {
        vec3 dirToLight = lightBuffer.light0.pos - inWorldPos;
        float distToLight = length(dirToLight);
        float attenuation = clamp(1.f / (0.01f + 0.01f * distToLight + 0.02f * distToLight * distToLight), 0.f, 1.f);
        vec3 L = normalize(dirToLight);

        // calculate per-light radiance
        vec3 H = normalize(V + L);
        vec3 radiance     = lightBuffer.light0.color * attenuation;
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = FresnelSchlick(max(dot(H, V), 0.0f), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metalness;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo.rgb / M_PI + specular) * radiance * NdotL; 
        ambientColor += albedo.xyz * lightBuffer.light0.ambient;
    }   

    vec3 color = ambientColor + Lo;
	
    /*
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   */
    outColor = vec4(color, 1.0f);
}