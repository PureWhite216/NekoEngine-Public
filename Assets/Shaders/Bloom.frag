#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(set = 0, binding = 2) uniform sampler2D u_BloomTexture;

layout(location = 0) in vec2 outTexCoord;

layout(push_constant) uniform Uniforms
{
    vec4 Params; // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
	vec4 Params2; //float LOD; // int Mode; // See defines below, (z) width, (w) height
} u_Uniforms;

#define MODE_PREFILTER      0
#define MODE_DOWNSAMPLE     1
#define MODE_UPSAMPLE_FIRST 2
#define MODE_UPSAMPLE       3

const float Epsilon = 1.0e-4;

float Luminance(vec3 rgb) 
{
    return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

vec3 Karis(vec3 hdr) 
{
    const float avg = 1.0f / (1.0f + (Luminance(hdr) * 0.25));
    return hdr * avg;
}


/// downsampleBox13 with Partial Karis Average
/// Uses the weighted avarage for the 5 h4x4box
vec4 DownsampleBox13Balanced(sampler2D tex, vec2 uv, vec2 pixelSize) {

	// .  .  .  .  .  .  .
	// .  A  .  B  .  C  .
	// .  .  D  .  E  .  .
	// .  F  .  G  .  H  .
	// .  .  I  .  J  .  .
	// .  K  .  L  .  M  .
	// .  .  .  .  .  .  .

	vec4 A = texture(tex, uv + pixelSize * vec2(-1.0, -1.0));
	vec4 B = texture(tex, uv + pixelSize * vec2(+0.0, -1.0));
	vec4 C = texture(tex, uv + pixelSize * vec2(+1.0, -1.0));
	vec4 D = texture(tex, uv + pixelSize * vec2(-0.5, -0.5));
	vec4 E = texture(tex, uv + pixelSize * vec2(+0.5, -0.5));
	vec4 F = texture(tex, uv + pixelSize * vec2(-1.0, +0.0));
	vec4 G = texture(tex, uv + pixelSize * vec2(+0.0, +0.0));
	vec4 H = texture(tex, uv + pixelSize * vec2(+1.0, +0.0));
	vec4 I = texture(tex, uv + pixelSize * vec2(-0.5, +0.5));
	vec4 J = texture(tex, uv + pixelSize * vec2(+0.5, +0.5));
	vec4 K = texture(tex, uv + pixelSize * vec2(-1.0, +1.0));
	vec4 L = texture(tex, uv + pixelSize * vec2(+0.0, +1.0));
	vec4 M = texture(tex, uv + pixelSize * vec2(+1.0, +1.0));

	vec4 hboxCC = (D + E + J + I) / 4.0;
	vec4 hboxTL = (A + B + G + F) / 4.0;
	vec4 hboxTR = (B + C + H + G) / 4.0;
	vec4 hboxBL = (F + G + L + K) / 4.0;
	vec4 hboxBR = (G + H + M + L) / 4.0;

	// 	weight = contribution * 1.0 / (1.0 + brigthness);
	float weightCC = 0.500 * 1.0 / (1.0 + max(hboxCC.r, max(hboxCC.g, hboxCC.b)));
	float weightTL = 0.125 * 1.0 / (1.0 + max(hboxTL.r, max(hboxTL.g, hboxTL.b)));
	float weightTR = 0.125 * 1.0 / (1.0 + max(hboxTR.r, max(hboxTR.g, hboxTR.b)));
	float weightBL = 0.125 * 1.0 / (1.0 + max(hboxBL.r, max(hboxBL.g, hboxBL.b)));
	float weightBR = 0.125 * 1.0 / (1.0 + max(hboxBR.r, max(hboxBR.g, hboxBR.b)));

	float weightSum = weightCC + weightTL + weightTR + weightBL + weightBR;

	return
			(weightCC / weightSum) * hboxCC +
			(weightTL / weightSum) * hboxTL +
			(weightTR / weightSum) * hboxTR +
			(weightBL / weightSum) * hboxBL +
			(weightBR / weightSum) * hboxBR;
}

vec3 DownsampleBox13(sampler2D tex, float lod, vec2 uv, vec2 texelSize)
{
    // Center
    vec3 A = textureLod(tex, uv, lod).rgb;
	
    texelSize *= 0.5f; // Sample from center of texels
    
	// Inner box
    vec3 B = textureLod(tex, uv + texelSize * vec2(-1.0f, -1.0f), lod).rgb;
    vec3 C = textureLod(tex, uv + texelSize * vec2(-1.0f, 1.0f), lod).rgb;
    vec3 D = textureLod(tex, uv + texelSize * vec2(1.0f, 1.0f), lod).rgb;
    vec3 E = textureLod(tex, uv + texelSize * vec2(1.0f, -1.0f), lod).rgb;
	
    // Outer box
    vec3 F = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 G = textureLod(tex, uv + texelSize * vec2(-2.0f, 0.0f), lod).rgb;
    vec3 H = textureLod(tex, uv + texelSize * vec2(0.0f, 2.0f), lod).rgb;
    vec3 I = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 J = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 K = textureLod(tex, uv + texelSize * vec2(2.0f, 0.0f), lod).rgb;
    vec3 L = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 M = textureLod(tex, uv + texelSize * vec2(0.0f, -2.0f), lod).rgb;
	
    // Weights
    vec3 result = vec3(0.0);
    {
        // Inner box
        result += (B + C + D + E) * 0.5f;
        // Bottom-left box
        result += (F + G + A + M) * 0.125f;
        // Top-left box
        result += (G + H + I + A) * 0.125f;
        // Top-right box
        result += (A + I + J + K) * 0.125f;
        // Bottom-right box
        result += (M + A + K + L) * 0.125f;
        
        // 4 samples each
        result *= 0.25f;
    }

    return result;
}

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
    // Maximum pixel brightness
    float brightness = max(max(color.r, color.g), color.b);
    // Quadratic curve
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = (rq * rq) * curve.z;
    color *= max(rq, brightness - threshold) / max(brightness, Epsilon);
    return color;
}

vec4 Prefilter(vec4 color, vec2 uv)
{
    float clampValue = 20.0f;
    color = min(vec4(clampValue), color);
    color = QuadraticThreshold(color, u_Uniforms.Params.x, u_Uniforms.Params.yzw);
    return color;
}

vec3 UpsampleTent9(sampler2D tex, float lod, vec2 uv, vec2 texelSize, float radius)
{
    vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;
	
    // Center
    vec3 result = textureLod(tex, uv, lod).rgb * 4.0f;
	
    result += textureLod(tex, uv - offset.xy, lod).rgb;
    result += textureLod(tex, uv - offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv - offset.zy, lod).rgb;
	
    result += textureLod(tex, uv + offset.zw, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xw, lod).rgb * 2.0;
	
    result += textureLod(tex, uv + offset.zy, lod).rgb;
    result += textureLod(tex, uv + offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xy, lod).rgb;
	
    return result * (1.0f / 16.0f);
}

void main()
{
    vec2 imgSize = u_Uniforms.Params2.zw;
	
    vec2 texCoords = outTexCoord;
	int LOD = int(u_Uniforms.Params2.x);
	int Mode = int(u_Uniforms.Params2.y);
    vec2 texSize = vec2(textureSize(u_Texture, LOD));
    
	//texCoords += (1.0f / imgSize) * 0.5f;
	
    vec4 color = vec4(1, 0, 1, 1);
	
    if (Mode == MODE_PREFILTER)
    {
        color.rgb = DownsampleBox13Balanced(u_Texture, texCoords, 1.0f / texSize).rgb;
        color = Prefilter(color, texCoords);
        color.a = 1.0f;
    }
    else if (Mode == MODE_UPSAMPLE_FIRST)
    {
        vec2 bloomTexSize = vec2(textureSize(u_Texture, LOD + 1));
        float sampleScale = 1.0f;
        vec3 upsampledTexture = UpsampleTent9(u_Texture, float(LOD) + 1.0f, texCoords, 1.0f / bloomTexSize, sampleScale);
		
        vec3 existing = textureLod(u_Texture, texCoords, LOD).rgb;
        color.rgb = existing + upsampledTexture;
    }
    else if (Mode == MODE_UPSAMPLE)
    {
        vec2 bloomTexSize = vec2(textureSize(u_BloomTexture, LOD + 1));
        float sampleScale = 1.0f;
        vec3 upsampledTexture = UpsampleTent9(u_BloomTexture, float(LOD) + 1.0f, texCoords, 1.0f / bloomTexSize, sampleScale);
		
        vec3 existing = textureLod(u_Texture, texCoords, LOD).rgb;
        color.rgb = existing + upsampledTexture;
    }
    else if (Mode == MODE_DOWNSAMPLE)
    {
        // Downsample
        color.rgb = DownsampleBox13(u_Texture, float(LOD), texCoords, 1.0f / texSize);
    }
    outColour = color;
}