#include "GLTFLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
//#define TINYGLTF_NO_STB_IMAGE
//#define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

#include <map>

namespace viewer {

aka::VertexType vertexType(int tinyGltfType)
{
	switch (tinyGltfType)
	{
	case TINYGLTF_TYPE_VEC2:
		return aka::VertexType::Vec2;
	case TINYGLTF_TYPE_VEC3:
		return aka::VertexType::Vec3;
	case TINYGLTF_TYPE_VEC4:
		return aka::VertexType::Vec4;
	case TINYGLTF_TYPE_MAT2:
		return aka::VertexType::Mat2;
	case TINYGLTF_TYPE_MAT3:
		return  aka::VertexType::Mat3;
	case TINYGLTF_TYPE_MAT4:
		return aka::VertexType::Mat4;
	case TINYGLTF_TYPE_SCALAR:
		return aka::VertexType::Scalar;
	default:
	case TINYGLTF_TYPE_VECTOR:
	case TINYGLTF_TYPE_MATRIX:
		aka::Logger::error("Vertex type not supported : ", tinyGltfType);
		return aka::VertexType::Scalar;
	}
}
aka::VertexFormat vertexFormat(int tinyGltfComponentType)
{
	switch (tinyGltfComponentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		return aka::VertexFormat::Byte;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return aka::VertexFormat::UnsignedByte;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		return aka::VertexFormat::Short;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return aka::VertexFormat::UnsignedShort;
	case TINYGLTF_COMPONENT_TYPE_INT:
		return aka::VertexFormat::Int;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return aka::VertexFormat::UnsignedInt;
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		return aka::VertexFormat::Float;
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		return aka::VertexFormat::Double;
	default:
		aka::Logger::error("Vertex format not supported : ", tinyGltfComponentType);
		return aka::VertexFormat::Float;
	}
}
aka::IndexFormat indexFormat(int tinyGltfComponentType)
{
	switch (tinyGltfComponentType)
	{
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return aka::IndexFormat::UnsignedByte;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return aka::IndexFormat::UnsignedShort;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return aka::IndexFormat::UnsignedInt;
	default:
	case TINYGLTF_COMPONENT_TYPE_BYTE:
	case TINYGLTF_COMPONENT_TYPE_SHORT:
	case TINYGLTF_COMPONENT_TYPE_INT:
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		aka::Logger::error("Index format not supported : ", tinyGltfComponentType);
		return aka::IndexFormat::UnsignedInt;
	}
}
aka::Sampler::Filter samplerFilter(int filter)
{
	switch (filter)
	{
	case -1:
		return aka::Sampler::Filter::Linear;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:
		return aka::Sampler::Filter::Linear;
	case TINYGLTF_TEXTURE_FILTER_NEAREST:
		return aka::Sampler::Filter::Nearest;
	default:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		return aka::Sampler::Filter::MipMapNearest;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		return aka::Sampler::Filter::MipMapLinear;
	}
}
aka::Sampler::Wrap samplerWrap(int wrap)
{
	switch (wrap)
	{
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		return aka::Sampler::Wrap::Repeat;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		return aka::Sampler::Wrap::Clamp;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		return aka::Sampler::Wrap::Mirror;
	default:
		aka::Logger::warn("Sampler wrap not supported : ", wrap);
		return aka::Sampler::Wrap::Repeat;
	}
}

Material convertMaterial(const tinygltf::Model& tinyModel, const tinygltf::Material& tinyMat)
{
	Material material;
	// Base color
	auto itColor = tinyMat.values.find("baseColorFactor");
	if (itColor == tinyMat.values.end())
		material.color = aka::color4f(1.f);
	else
	{
		tinygltf::ColorValue color = itColor->second.ColorFactor();
		material.color = aka::color4f(
			static_cast<float>(color[0]),
			static_cast<float>(color[1]),
			static_cast<float>(color[2]),
			static_cast<float>(color[3])
		);
	}
	// Color texture
	auto itTexture = tinyMat.values.find("baseColorTexture");
	if (itTexture != tinyMat.values.end())
	{
		auto itIndex = itTexture->second.json_double_value.find("index");
		ASSERT(itIndex != itTexture->second.json_double_value.end(), "Index not defined");
		const unsigned int index = static_cast<unsigned int>(itIndex->second);
		const tinygltf::Texture& tinyTexture = tinyModel.textures[index];
		const tinygltf::Image& tinyImage = tinyModel.images[tinyTexture.source];
		ASSERT(tinyImage.bits == 8, "");
		aka::Sampler sampler{};
		if (tinyTexture.sampler > -1)
		{
			const tinygltf::Sampler& tinySampler = tinyModel.samplers[tinyTexture.sampler];
			sampler.filterMag = samplerFilter(tinySampler.magFilter);
			sampler.filterMin = samplerFilter(tinySampler.minFilter);
			sampler.wrapS = samplerWrap(tinySampler.wrapS);
			sampler.wrapT = samplerWrap(tinySampler.wrapT);
		}
		else
		{
			sampler.filterMag = aka::Sampler::Filter::Nearest;
			sampler.filterMin = aka::Sampler::Filter::Nearest;
			sampler.wrapS = aka::Sampler::Wrap::Repeat;
			sampler.wrapT = aka::Sampler::Wrap::Repeat;
		}
		material.colorTexture = aka::Texture::create(
			tinyImage.width,
			tinyImage.height,
			tinyImage.component == 4 ? aka::Texture::Format::Rgba : aka::Texture::Format::Rgb,
			tinyImage.image.data(),
			sampler
		);
	}
	else
	{
		uint8_t data[4] = { 255,255,255,255 };
		aka::Sampler sampler;
		sampler.filterMag = aka::Sampler::Filter::Nearest;
		sampler.filterMin = aka::Sampler::Filter::Nearest;
		sampler.wrapS = aka::Sampler::Wrap::Repeat;
		sampler.wrapT = aka::Sampler::Wrap::Repeat;
		material.colorTexture = aka::Texture::create(1U, 1U, aka::Texture::Format::Rgba, data, sampler);
	}
	// Culling
	if (tinyMat.doubleSided)
	{
		material.doubleSided = true;
	}
	else
	{
		material.doubleSided = false;
	}
	tinyMat.alphaCutoff;
	tinyMat.alphaMode; // OPAQUE, BLEND, MASK (use alphaCutOff)
	if (tinyMat.normalTexture.index > 0)
	{
		const tinygltf::Texture& tinyTexture = tinyModel.textures[tinyMat.normalTexture.index];
		const tinygltf::Image& tinyImage = tinyModel.images[tinyTexture.source];
		ASSERT(tinyImage.bits == 8, "");
		aka::Sampler sampler{};
		if (tinyTexture.sampler > -1)
		{
			const tinygltf::Sampler& tinySampler = tinyModel.samplers[tinyTexture.sampler];
			sampler.filterMag = samplerFilter(tinySampler.magFilter);
			sampler.filterMin = samplerFilter(tinySampler.minFilter);
			sampler.wrapS = samplerWrap(tinySampler.wrapS);
			sampler.wrapT = samplerWrap(tinySampler.wrapT);
		}
		else
		{
			sampler.filterMag = aka::Sampler::Filter::Nearest;
			sampler.filterMin = aka::Sampler::Filter::Nearest;
			sampler.wrapS = aka::Sampler::Wrap::Repeat;
			sampler.wrapT = aka::Sampler::Wrap::Repeat;
		}
		material.normalTexture = aka::Texture::create(
			tinyImage.width,
			tinyImage.height,
			tinyImage.component == 4 ? aka::Texture::Format::Rgba : aka::Texture::Format::Rgb,
			tinyImage.image.data(),
			sampler
		);
	}
	else
	{
		uint8_t data[4] = { 128,255,128,255 };
		aka::Sampler sampler;
		sampler.filterMag = aka::Sampler::Filter::Nearest;
		sampler.filterMin = aka::Sampler::Filter::Nearest;
		sampler.wrapS = aka::Sampler::Wrap::Repeat;
		sampler.wrapT = aka::Sampler::Wrap::Repeat;
		material.normalTexture = aka::Texture::create(1U, 1U, aka::Texture::Format::Rgba, data, sampler);
	}
	return material;
}

struct StridedData {
	size_t count = 0;
	size_t stride = 0;
	const uint8_t* data = nullptr;
	const uint8_t* get(size_t i) const { return data + stride * i; }
};

StridedData convertAttribute(const std::string & name, const VertexData::Attribute &attribute, const tinygltf::Model& tinyModel, const tinygltf::Primitive& primitive)
{
	auto it = primitive.attributes.find(name);
	if (it == primitive.attributes.end())
	{
		aka::Logger::warn("Attribute not supported : ", name);
		return StridedData();
	}
	const tinygltf::Accessor& accessor = tinyModel.accessors[it->second];
	ASSERT(attribute.type == vertexType(accessor.type), "Incorrect type");
	ASSERT(attribute.format == vertexFormat(accessor.componentType), "Incorrect format");
	const tinygltf::BufferView& bufferView = tinyModel.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = tinyModel.buffers[bufferView.buffer];
	StridedData stridedData;
	stridedData.data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
	stridedData.stride = accessor.ByteStride(bufferView);
	stridedData.count = accessor.count;
	return stridedData;
}

std::vector<norm3f> generateNormal(const vec3f *vertices, size_t vertexCount, const void* voidIndices, IndexFormat index, size_t indexCount)
{
	std::vector<norm3f> normals(vertexCount);
	if (voidIndices != nullptr)
	{
		ASSERT(indexCount % 3 == 0, "Incomplete triangles");
		switch (index)
		{
		case IndexFormat::UnsignedByte: {
			const uint8_t* indices = reinterpret_cast<const uint8_t*>(voidIndices);
			for (size_t i = 0; i < indexCount / 3; i++)
			{
				const vec3f& v0 = vertices[indices[i * 3 + 0]];
				const vec3f& v1 = vertices[indices[i * 3 + 1]];
				const vec3f& v2 = vertices[indices[i * 3 + 2]];
				const norm3f geoNormal = norm3f::normalize(norm3f(vec3f::cross(v1 - v0, v2 - v0)));
				normals[indices[i * 3 + 0]] = geoNormal;
				normals[indices[i * 3 + 1]] = geoNormal;
				normals[indices[i * 3 + 2]] = geoNormal;
			}
			break;
		}
		case IndexFormat::UnsignedShort: {
			const uint16_t* indices = reinterpret_cast<const uint16_t*>(voidIndices);
			for (size_t i = 0; i < indexCount / 3; i++)
			{
				const vec3f& v0 = vertices[indices[i * 3 + 0]];
				const vec3f& v1 = vertices[indices[i * 3 + 1]];
				const vec3f& v2 = vertices[indices[i * 3 + 2]];
				const norm3f geoNormal = norm3f::normalize(norm3f(vec3f::cross(v1 - v0, v2 - v0)));
				normals[indices[i * 3 + 0]] = geoNormal;
				normals[indices[i * 3 + 1]] = geoNormal;
				normals[indices[i * 3 + 2]] = geoNormal;
			}
			break;
		}
		case IndexFormat::UnsignedInt: {
			const uint32_t* indices = reinterpret_cast<const uint32_t*>(voidIndices);
			for (size_t i = 0; i < indexCount / 3; i++)
			{
				const vec3f& v0 = vertices[indices[i * 3 + 0]];
				const vec3f& v1 = vertices[indices[i * 3 + 1]];
				const vec3f& v2 = vertices[indices[i * 3 + 2]];
				const norm3f geoNormal = norm3f::normalize(norm3f(vec3f::cross(v1 - v0, v2 - v0)));
				normals[indices[i * 3 + 0]] = geoNormal;
				normals[indices[i * 3 + 1]] = geoNormal;
				normals[indices[i * 3 + 2]] = geoNormal;
			}
			break;
		}
		default:
			Logger::error("Unsupported type. Undefined normals.");
			break;
		}
	}
	else
	{
		ASSERT(vertexCount % 3 == 0, "Incomplete triangles");
		for (size_t i = 0; i < vertexCount / 3; i++)
		{
			const vec3f& v0 = vertices[i * 3 + 0];
			const vec3f& v1 = vertices[i * 3 + 1];
			const vec3f& v2 = vertices[i * 3 + 2];
			const norm3f geoNormal = norm3f::normalize(norm3f(vec3f::cross(v1 - v0, v2 - v0)));
			normals[i * 3 + 0] = geoNormal;
			normals[i * 3 + 1] = geoNormal;
			normals[i * 3 + 2] = geoNormal;
		}
	}
	return normals;
}

Mesh::Ptr convertMesh(const tinygltf::Model& tinyModel, const tinygltf::Primitive& primitive, BoundingBox &bbox, const mat4f &t)
{
	Mesh::Ptr mesh = Mesh::create();
	// Vertex
	VertexData vertData;
	vertData.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
	vertData.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
	vertData.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
	vertData.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });
	std::vector<Vertex> vertices; // TODO do not force a layout.
	// Index
	aka::IndexFormat index = aka::IndexFormat::UnsignedByte;
	const void* indices = nullptr;
	size_t indexCount = 0;
	{
		// Indices
		if (primitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor = tinyModel.accessors[primitive.indices];
			ASSERT(accessor.type == TINYGLTF_TYPE_SCALAR, "Only scalar supported for index");
			const tinygltf::BufferView& bufferView = tinyModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = tinyModel.buffers[bufferView.buffer];
			const unsigned char *data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
			index = indexFormat(accessor.componentType);
			indexCount = accessor.count;
			indices = data;
		}
	}
	{
		// --- Vertices
		StridedData stridedData[4];
		stridedData[0] = convertAttribute("POSITION", vertData.attributes[0], tinyModel, primitive);
		stridedData[1] = convertAttribute("NORMAL", vertData.attributes[1], tinyModel, primitive);
		stridedData[2] = convertAttribute("TEXCOORD_0", vertData.attributes[2], tinyModel, primitive);
		stridedData[3] = convertAttribute("COLOR_0", vertData.attributes[3], tinyModel, primitive);
		size_t count = stridedData[0].count;
		// --- Fill empty data
		ASSERT(stridedData[0].data != nullptr, "No position set !");
		std::vector<norm3f> normals;
		std::vector<uv2f> uvs;
		std::vector<color4f> colors;
		// Normal
		if (stridedData[1].data == nullptr)
		{
			normals = generateNormal(reinterpret_cast<const vec3f*>(stridedData[0].data), stridedData[0].count, indices, index, indexCount);
			stridedData[1].data = reinterpret_cast<uint8_t*>(normals.data());
			stridedData[1].count = normals.size();
			stridedData[1].stride = sizeof(vec3f);
		}
		// UV
		if (stridedData[2].data == nullptr)
		{
			uvs.resize(count, uv2f(0.f));
			stridedData[2].data = reinterpret_cast<uint8_t*>(uvs.data());
			stridedData[2].count = uvs.size();
			stridedData[2].stride = sizeof(uv2f);
		}
		// Color
		if (stridedData[3].data == nullptr)
		{
			colors.resize(count, color4f(1.f));
			stridedData[3].data = reinterpret_cast<uint8_t*>(colors.data());
			stridedData[3].count = colors.size();
			stridedData[3].stride = sizeof(color4f);
		}

		// --- Create vertex
		vertices.resize(count);
		for (size_t iVert = 0; iVert < count; iVert++)
		{
			Vertex& v = vertices[iVert];
			memcpy(v.position.data, stridedData[0].get(iVert), sizeof(float) * 3);
			memcpy(v.normal.data, stridedData[1].get(iVert), sizeof(float) * 3);
			memcpy(v.uv.data, stridedData[2].get(iVert), sizeof(float) * 2);
			memcpy(v.color.data, stridedData[3].get(iVert), sizeof(float) * 4);
			bbox.include(t.multiplyPoint(v.position));
		}
	}
	mesh->vertices(vertData, vertices.data(), vertices.size());
	if (indices != nullptr)
		mesh->indices(index, indices, indexCount);
	return mesh;
}

// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
void convertNode(Model::Ptr model, const tinygltf::Model& tinyModel, const tinygltf::Node& node, const aka::mat4f& transform)
{
	aka::mat4f t = transform;
	if (node.matrix.size() == 16)
	{
		t *= aka::mat4f(
			aka::col4f(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3]),
			aka::col4f(node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7]),
			aka::col4f(node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11]),
			aka::col4f(node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15])
		);
	}
	else if (node.translation.size() > 0 || node.rotation.size() > 0 || node.scale.size() > 0)
	{
		vec3f translation(0.f);
		vec3f scale(1.f);
		quatf rotation = quatf::identity();
		if (node.translation.size() == 3)
			translation = aka::vec3f(node.translation[0], node.translation[1], node.translation[2]);
		if (node.rotation.size() == 4)
			rotation = aka::quatf(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
		if (node.scale.size() == 3)
			scale = aka::vec3f(node.scale[0], node.scale[1], node.scale[2]);
		t *= mat4f::TRS(translation, rotation, scale);
	}
	if (node.camera != -1)
	{
		aka::Logger::warn("Camera not suppored");
	}
	if (node.mesh != -1)
	{
		const tinygltf::Mesh& tinyMesh = tinyModel.meshes[node.mesh];
		for (const tinygltf::Primitive& primitive : tinyMesh.primitives)
		{
			const tinygltf::Material& tinyMat = tinyModel.materials[primitive.material];
			ASSERT(primitive.mode == TINYGLTF_MODE_TRIANGLES, "Not supported");
			aka::Mesh::Ptr mesh = convertMesh(tinyModel, primitive, model->bbox, t);
			Material material = convertMaterial(tinyModel, tinyMat);

			model->materials.push_back(material);
			model->meshes.push_back(mesh);
			model->transforms.push_back(t);
		}
	}
	for (const int& childNodeID : node.children)
		convertNode(model, tinyModel, tinyModel.nodes[childNodeID], t);
}

Model::Ptr viewer::GLTFLoader::load(const Path& path)
{
	tinygltf::Model tinyModel;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	/*loader.SetImageLoader([](
		tinygltf::Image* tinyImage, 
		const int imageID,
		std::string* err, 
		std::string* warn , 
		int reqWidth, 
		int reqHeight, 
		const unsigned char* bytes, 
		int size, 
		void* userData
	) -> bool {
		Image image = Image::load(bytes, size);
		if (image.bytes.size() == 0)
		{
			*err = "Unknown image format. Cannot decode image data for image[" + std::to_string(imageID) + "] name = \"" + tinyImage->name + "\".\n";
			return false;
		}
		if (reqWidth > 0 && image.width != reqWidth)
		{
			*err = "Image width mismatch. for image[" + std::to_string(imageID) + "] name = \"" + tinyImage->name + "\"\n";
			return false;
		}
		if (reqHeight > 0 && image.height != reqHeight)
		{
			*err = "Image height mismatch. for image[" + std::to_string(imageID) + "] name = \"" + tinyImage->name + "\"\n";
			return false;
		}
		tinyImage->width = image.width;
		tinyImage->height = image.height;
		tinyImage->component = 4;
		tinyImage->bits = 8;
		tinyImage->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		tinyImage->image = std::move(image.bytes);
		return true;
	}, nullptr);*/
	bool ret = loader.LoadASCIIFromFile(&tinyModel, &err, &warn, path.str());
	if (!warn.empty())
		aka::Logger::warn(warn);
	if (!err.empty())
	{
		aka::Logger::error(err);
		return nullptr;
	}
	if (!ret)
	{
		aka::Logger::critical("Failed to load glTF");
		return nullptr;
	}

	tinygltf::Scene& defaultScene = tinyModel.scenes[tinyModel.defaultScene];
	Model::Ptr model = std::make_shared<Model>();
	for (int& nodeID : defaultScene.nodes)
		convertNode(model, tinyModel, tinyModel.nodes[nodeID], aka::mat4f::identity());
	return model;
}

};