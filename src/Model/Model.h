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

struct Camera3DController {
	virtual mat4f transform() const = 0;
	virtual mat4f view() const = 0;
	virtual void set(const aabbox<>& bbox) {}
};
// CameraProjectionComponent
struct Camera3DComponent {
	mat4f view;
	CameraProjection* projection;
	//Camera3DController* controller;
};

struct DirtyLightComponent {};

class SceneGraph : public System
{
public:
	void create(World& world) override;
	void destroy(World& world) override;
	// The update is deferred.
	void update(World& world, Time::Unit deltaTime) override;
};

class ArcballCamera : Camera3DController
{
public:
	ArcballCamera();
	ArcballCamera(const aabbox<>& bbox);

	// Rotate the camera
	void rotate(float x, float y);
	// Pan the camera
	void pan(float x, float y);
	// Zoom the camera
	void zoom(float zoom);

	// Set the camera bounds
	void set(const aabbox<>& bbox);
	// Update the camera depending on user input
	void update(Time::Unit deltaTime);

	mat4f transform() const override { return m_transform; }
	mat4f view() const override { return mat4f::inverse(m_transform); }
private:
	mat4f m_transform;
	float m_speed;
	point3f m_position;
	point3f m_target;
	norm3f m_up;
};

};