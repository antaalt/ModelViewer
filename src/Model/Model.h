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
	Texture::Ptr roughnessTexture;
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
	static constexpr float far = 40.f;
	mat4f worldToLightSpaceMatrix[6];
	Texture::Ptr shadowMap;
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

class Scene
{
public:
	class GraphSystem : public System
	{
	public:
		void create(World& world) override;
		void destroy(World& world) override;
		// The update is deferred.
		void update(World& world, Time::Unit deltaTime) override;
	};
	static Entity getMainCamera(World& world) { return Entity::null(); }
	// Factory
	static Entity createCubeMesh(World& world);
	static Entity createSphereMesh(World& world, uint32_t segmentCount, uint32_t ringCount);
	static Entity createPointLight(World& world);
	static Entity createDirectionalLight(World& world);
	static Entity createArcballCamera(World& world, CameraProjection* projection);
};

class ArcballCameraSystem : public System
{
public:
	void update(World& world, Time::Unit deltaTime) override;
};

};