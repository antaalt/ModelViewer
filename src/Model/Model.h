#pragma once

#include <Aka/Aka.h>

namespace viewer {

using namespace aka;

struct Transform3DComponent {
	mat4f transform;
};

struct Hierarchy3DComponent {
	Entity parent;
	mat4f inverseTransform;
};

struct MeshComponent {
	SubMesh submesh;
	aabbox<> bounds;
};

struct MaterialComponent {
	color4f color;
	bool doubleSided;
	Texture::Ptr colorTexture;
	Texture::Ptr normalTexture;
	Texture::Ptr materialTexture;
	//Texture::Ptr emissiveTexture;
};

// CSM based directional light
struct DirectionalLightComponent {
	vec3f direction;
	color3f color;
	float intensity;
	// Rendering
	static constexpr size_t cascadeCount = 3;
	mat4f worldToLightSpaceMatrix[cascadeCount];
	Texture::Ptr shadowMap[cascadeCount];
	float cascadeEndClipSpace[cascadeCount];
};

struct PointLightComponent {
	color3f color;
	float intensity;
	// Rendering
	mat4f worldToLightSpaceMatrix[6];
	Texture::Ptr shadowMap;
	float radius;
};

/*struct Camera3DController {
	virtual mat4f transform() const = 0;
	virtual mat4f view() const = 0;
	virtual void set(const aabbox<>& bbox) {}
};*/
// CameraProjectionComponent
struct Camera3DComponent {
	mat4f view;
	CameraProjection* projection;
	//Camera3DController* controller;
};

struct ArcballCameraComponent {
	point3f position;
	point3f target;
	norm3f up;

	float speed;
	bool active;

	void set(const aabbox<>& bbox);
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
	static Entity createArcballCameraEntity(World& world, CameraProjection* projection);

	static void save(const Path& path, const World& world);
	static void load(World& world, const Path& path);
};

};