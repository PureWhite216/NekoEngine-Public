#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct VertexData
{
	vec3 Colour;
	vec2 TexCoord;
	vec4 Position;
	vec3 Normal;
	mat3 WorldNormal;
};

layout(location = 0) in VertexData VertexOutput;

layout(set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D u_MetallicMap;
layout(set = 1, binding = 2) uniform sampler2D u_RoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D u_NormalMap;
layout(set = 1, binding = 4) uniform sampler2D u_AOMap;
layout(set = 1, binding = 5) uniform sampler2D u_EmissiveMap;

layout(set = 1,binding = 6) uniform UniformMaterialData
{
	vec4  AlbedoColour;
	float Roughness;
	float Metallic;
	float Emissive;
	float AlbedoMapFactor;
	float MetallicMapFactor;
	float RoughnessMapFactor;
	float NormalMapFactor;
	float AOMapFactor;
	float EmissiveMapFactor;
	float AlphaCutOff;
	float workflow;
	float padding;
} materialProperties;

layout(location = 0) out vec4 OutNormal;
void main(void)
{
	float alpha = texture(u_AlbedoMap, VertexOutput.TexCoord).a;
	
	if(alpha < materialProperties.AlphaCutOff)
		discard;

	OutNormal = vec4(normalize(VertexOutput.Normal * 0.5 + 0.5), 1.0f);
}