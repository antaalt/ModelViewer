#version 330

layout(location = 0) out vec4 o_color;

const int SHADOW_CASCADE_COUNT = 3;
const float PI = 3.14159265359;

in vec2 v_uv;

uniform sampler2D u_position;
uniform sampler2D u_albedo;
uniform sampler2D u_normal;
uniform sampler2D u_depth;
uniform sampler2D u_roughness;
uniform samplerCube u_skybox;

uniform vec3 u_cameraPos;
uniform float u_farPointLight;

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
	mat4 worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
	float cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
	sampler2D shadowMap[SHADOW_CASCADE_COUNT];
};

struct PointLight {
	vec3 position;
	float intensity;
	vec3 color;
	samplerCube shadowMap;
};

uniform int u_dirLightCount;
uniform int u_pointLightCount;

const int MAX_DIR_LIGHT_COUNT = 4;
uniform DirectionalLight u_dirLights[MAX_DIR_LIGHT_COUNT];

const int MAX_POINT_LIGHT_COUNT = 8;
uniform PointLight u_pointLights[MAX_POINT_LIGHT_COUNT];

vec2 poissonDisk[16] = vec2[](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 ),
	vec2( -0.91588581, 0.45771432 ),
	vec2( -0.81544232, -0.87912464 ),
	vec2( -0.38277543, 0.27676845 ),
	vec2( 0.97484398, 0.75648379 ),
	vec2( 0.44323325, -0.97511554 ),
	vec2( 0.53742981, -0.47373420 ),
	vec2( -0.26496911, -0.41893023 ),
	vec2( 0.79197514, 0.19090188 ),
	vec2( -0.24188840, 0.99706507 ),
	vec2( -0.81409955, 0.91437590 ),
	vec2( 0.19984126, 0.78641367 ),
	vec2( 0.14383161, -0.14100790 )
);

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

vec3 computePointShadows(vec3 from, int lightID)
{
	vec3 fragToLight = from - u_pointLights[lightID].position;
	float currentDepth = length(fragToLight);
	float shadow  = 0.0;
	float bias    = 0.15;
	float samples = 20;
	float diskRadius = 0.05;
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(u_pointLights[lightID].shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= u_farPointLight;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);
	return vec3(1.0 - shadow);
}

vec3 computeDirectionalShadows(vec3 from, int lightID)
{
	const float bias = 0.0003; // Low value to avoid peter panning
	vec3 visibility = vec3(1);
	float diffusion[3] = float[](1000.f, 3000.f, 5000.f); // TODO parameter
	for (int iCascade = 0; iCascade < SHADOW_CASCADE_COUNT; iCascade++)
	{
		// Small offset to blend cascades together smoothly
		float offset = random(v_uv.xyy, iCascade) * 0.0001;
		if (texture(u_depth, v_uv).x <= u_dirLights[lightID].cascadeEndClipSpace[iCascade] + offset)
		{
#if 0 // Debug cascades
			visibility = vec3(
				(iCascade == 0) ? 1.0 : 0.0,
				(iCascade == 1) ? 1.0 : 0.0,
				(iCascade == 2) ? 1.0 : 0.0
			);
#else
			// PCF
			const int pass = 16;
			vec4 lightTextureSpace = u_dirLights[lightID].worldToLightTextureSpace[iCascade] * vec4(from, 1.0);
			lightTextureSpace /= lightTextureSpace.w;
			for (int iPoisson = 0; iPoisson < 16; iPoisson++)
			{
				int index = int(16.0 * random(v_uv.xyy, iPoisson)) % 16;
				vec2 uv = lightTextureSpace.xy + poissonDisk[index] / diffusion[iCascade];
				if (texture(u_dirLights[lightID].shadowMap[iCascade], uv).z < lightTextureSpace.z - bias)
				{
					visibility -= 1.f / pass;
				}
			}
#endif
			break;
		}
	}
	return visibility;
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

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return vec3(clamp((x*(a*x+b))/(x*(c*x+d)+e), 0, 1));
}

void main(void)
{
	vec3 position = texture(u_position, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normal, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedo, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_roughness, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	vec3 N = normalize(normal);
	vec3 V = normalize(u_cameraPos - position);
	vec3 I = -V;

	vec3 Lo = vec3(0.0);
	// Point lights
	for(int i = 0; i < u_pointLightCount; ++i)
	{
		// Shadow
		vec3 visibility = computePointShadows(position, i);

	 	// Shading
		vec3 L = normalize(u_pointLights[i].position - position);
		vec3 H = normalize(V + L);

		float distance    = length(u_pointLights[i].position - position);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance     = u_pointLights[i].color* u_pointLights[i].intensity * attenuation;

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
		Lo += (kD * albedo / PI + specular) * radiance * NdotL * visibility;
	}
	// Directional lights
	for(int i = 0; i < u_dirLightCount; ++i)
	{
		// Shadow
		vec3 visibility = computeDirectionalShadows(position, i);

		// Shading
		vec3 L = normalize(u_dirLights[i].direction);
		vec3 H = normalize(V + L);

		float distance = 1.0;
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = u_dirLights[i].color * u_dirLights[i].intensity * attenuation;

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
		Lo += (kD * albedo / PI + specular) * radiance * NdotL * visibility;
	}

	// Reflection
	vec3 reflection = texture(u_skybox, reflect(I, N)).rgb;

	// Shading
	vec3 indirect = 0.03 * albedo; // ao broken for some
	vec3 direct = Lo;
	vec3 color = indirect + direct + reflection * 0.01; // TODO use irradiance map

	// Tonemapping
	// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	color = ACESFilm(color); // ACES approximation tonemapping
	//color = color / (color + vec3(1.0)); // Reinhard operator

	// Gamma correction
	color = pow(color, vec3(1.0/2.2));

	o_color = vec4(color, 1);
}
