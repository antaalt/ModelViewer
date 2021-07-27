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

struct DirectionnalLightComponent {
	vec3f direction;
	color3f color;
	float intensity;
};

struct PointLightComponent {
	color3f color;
	float intensity;
};

struct Camera3DComponent {
	//CameraProjection* projection;
	//Camera3DController* controller;
};

struct Camera3DController {
	// arcball, standard
};

struct CameraProjection {
	// either persp or ortho
};

class SceneGraph : public System
{
public:
	void create(World& world) override;
	void destroy(World& world) override;
	// The update is deferred.
	void update(World& world, Time::Unit deltaTime) override;
};

class ArcballCamera
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

	mat4f& transform() { return m_transform; }
	const mat4f& transform() const { return m_transform; }
private:
	mat4f m_transform;
	float m_speed;
	point3f m_position;
	point3f m_target;
	norm3f m_up;
};

};