#pragma once

#include <Aka/Aka.h>

namespace app {

using namespace aka;

struct Transform3DComponent {
	mat4f transform;
};

struct Hierarchy3DComponent {
	Entity parent;
	mat4f inverseTransform;
};

struct StaticMeshComponent {
	SubMesh submesh;
	aabbox<> bounds;
};

struct OpaqueMaterialComponent {
	struct Texture {
		aka::Texture::Ptr texture;
		TextureSampler sampler;
	};
	color4f color;
	bool doubleSided;
	Texture albedo;
	Texture normal;
	Texture material;
	//Texture::Ptr emissive;
};

using MeshComponent = StaticMeshComponent;
using MaterialComponent = OpaqueMaterialComponent;

// CSM based directional light
struct DirectionalLightComponent {
	vec3f direction;
	color3f color;
	float intensity;
	// Rendering
	static constexpr size_t cascadeCount = 3;
	mat4f worldToLightSpaceMatrix[cascadeCount];
	Texture2D::Ptr shadowMap[cascadeCount];
	float cascadeEndClipSpace[cascadeCount];
};

struct PointLightComponent {
	color3f color;
	float intensity;
	// Rendering
	mat4f worldToLightSpaceMatrix[6];
	TextureCubeMap::Ptr shadowMap;
	float radius;
};

// with a preetham component or something, we can emulate sky
// a system read all preetham skybox and draw on the the sky if update.
// it also control a directional light attached to it that render the sun with it.
struct SkyboxComponent {
	TextureCubeMap::Ptr skybox;
};

struct Camera3DComponent {
	mat4f view;
	std::unique_ptr<CameraProjection> projection;
	std::unique_ptr<CameraController> controller;
	bool active;
};

struct TextComponent
{
	Font::Ptr font;
	TextureSampler sampler;
	String text;
	color4f color;
};

struct DirtyLightComponent {};
struct DirtyCameraComponent {};
struct DirtyTransformComponent {};

struct Scene
{
	static Entity getMainCamera(World& world);
	// Factory
	static Mesh::Ptr createCubeMesh(const point3f& position, float size);
	static Mesh::Ptr createSphereMesh(const point3f& position, float radius, uint32_t segmentCount, uint32_t ringCount);

	static Entity createCubeEntity(World& world);
	static Entity createSphereEntity(World& world, uint32_t segmentCount, uint32_t ringCount);
	static Entity createPointLightEntity(World& world);
	static Entity createDirectionalLightEntity(World& world);
	static Entity createArcballCameraEntity(World& world);

	static void save(const Path& path, const World& world);
	static void load(World& world, const Path& path);
};

};