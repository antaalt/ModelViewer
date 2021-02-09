#pragma once

#include <Aka/Aka.h>

namespace viewer {

using namespace aka;

struct BoundingBox
{
	BoundingBox();

	void include(const point3f& point);
	void include(const BoundingBox& bbox);
	bool contains(const point3f& point) const;
	bool contains(const BoundingBox& aabbox) const;
	point3f center() const;
	vec3f extent() const;
	bool valid() const;

	point3f min;
	point3f max;
};

struct Material {
	using Ptr = std::shared_ptr<Material>;
	bool doubleSided;
	color4f color;
	float metallic;
	float roughness;
	Texture::Ptr colorTexture;
	Texture::Ptr normalTexture;
	Texture::Ptr metallicTexture;
	Texture::Ptr emissiveTexture;
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
	Material::Ptr material;

};

/*struct Primitive
{
	uint32_t indexOffset;
	uint32_t indexCount;
	Mesh::Ptr mesh;
};*/

struct Model
{
	using Ptr = std::shared_ptr<Model>;

	BoundingBox bbox;
	std::vector<Material> materials;
	//std::vector<Primitive> primitives;
	std::vector<mat4f> transforms;
	std::vector<Mesh::Ptr> meshes;
};

struct ArcballCamera
{
	mat4f transform;
	float speed = 1.f;

	point3f position;
	point3f target;
	norm3f up;

	void set(const BoundingBox& bbox);
	void update();
};

};