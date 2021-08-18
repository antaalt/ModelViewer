#version 330

layout(location = 0) out vec4 o_color;

const float PI = 3.14159265359;

#if 0 // When using quad.vert
in vec2 v_uv;
#else // When using point.vert
uniform vec2 u_screen;
#define v_uv (gl_FragCoord.xy / u_screen)
#endif

uniform sampler2D u_positionTexture;
uniform sampler2D u_albedoTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_materialTexture;

uniform vec3 u_cameraPos; // use u_view
uniform float u_farPointLight;
uniform vec3 u_lightPosition;
uniform float u_lightIntensity;
uniform vec3 u_lightColor;
uniform samplerCube u_shadowMap;

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

vec3 computePointShadows(vec3 from)
{
	vec3 fragToLight = from - u_lightPosition;
	float currentDepth = length(fragToLight);
	float shadow  = 0.0;
	float bias    = 0.15;
	float samples = 20;
	float diskRadius = 0.05;
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(u_shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= u_farPointLight;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);
	return vec3(1.0 - shadow);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a      = roughness*roughness;
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float num   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = GeometrySchlickGGX(NdotV, roughness);
	float ggx1  = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

void main(void)
{
	vec3 position = texture(u_positionTexture, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normalTexture, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedoTexture, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_materialTexture, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	vec3 N = normalize(normal);
	vec3 V = normalize(u_cameraPos - position);
	vec3 I = -V;

	// Shadow
	vec3 visibility = computePointShadows(position);

 	// Shading
	vec3 L = normalize(u_lightPosition - position);
	vec3 H = normalize(V + L);

	float distance    = length(u_lightPosition - position);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance     = u_lightColor * u_lightIntensity * attenuation;

	// non metallic surface always 0.04
	vec3 F0 = vec3(0.04);
	F0      = mix(F0, albedo, vec3(metalness));
	vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);

	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular     = numerator / max(denominator, 0.001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;

	float NdotL = max(dot(N, L), 0.0);
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * visibility;

	o_color = vec4(Lo, 1);
}
