#pragma once

#include <Aka/Aka.h>

namespace viewer {

using namespace aka;

struct Material {
	using Ptr = std::shared_ptr<Material>;
	bool doubleSided;
	color4f color;
	Texture::Ptr colorTexture;
	Texture::Ptr normalTexture;
	Texture::Ptr roughnessTexture;
	//Texture::Ptr emissiveTexture;
};

struct Vertex {
	point3f position;
	norm3f normal;
	uv2f uv;
	color4f color;
};

struct Node {
	mat4f transform;
	Mesh::Ptr mesh;
	Material material;
};

struct Model
{
	using Ptr = std::shared_ptr<Model>;

	aabbox<> bbox;
	std::vector<Node> nodes;
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