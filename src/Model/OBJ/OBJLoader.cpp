#include "OBJLoader.h"

#include <string>
#include <sstream>
#include <array>
#include <map>
#include <fstream>
#include <memory>

#include <Aka/OS/Logger.h>
#include <Aka/OS/Image.h>

namespace viewer {

struct OBJVertex {
	uint32_t posID;
	uint32_t normID;
	uint32_t uvID;
};

struct OBJFace {
	std::vector<OBJVertex> vertices;
	OBJVertex&operator[](size_t iVert) { return vertices[iVert]; }
	const OBJVertex&operator[](size_t iVert) const { return vertices[iVert]; }
};

// https://www.fileformat.info/format/material/index.htm
struct OBJMaterial {
	std::string name;
	vec3f Ka; // ambiant color
	vec3f Kd; // diffuse color
	vec3f Ks; // specular color
	vec3f Ke; // emissive color
	float Ni; // optic density
	float Ns; // specular exponent
	float d; // transparency [0, 1], 1 = no transparency
	unsigned int illum; // light parameters
	std::string map_Kd;
	std::string map_Ka;
	std::string map_Disp;
	std::string map_bump;
};

struct OBJGroup {
	std::string name;
	std::vector<OBJFace> faces;
	std::vector<Material::Ptr> materials;
};

struct OBJObject {
	std::string name;
	std::vector<OBJGroup> groups;
};


void skipWhitespace(std::stringstream &ss)
{
	char whitespaces[] = { ' ', '\t' };
	do
	{
		int c = ss.peek();
		for (char &whitespace : whitespaces)
		{
			if (c == whitespace)
			{
				ss.ignore(1);
				continue;
			}
		}
		break;
	} while (true);
}

void parseMaterials(const aka::Path& fileName, std::map<std::string, std::unique_ptr<OBJMaterial>> &materials) {
#if defined(AKA_PLATFORM_WINDOWS)
	aka::StrWide wstr = aka::encoding::wide(fileName.str());
	std::ifstream file(wstr.cstr());
#else
	std::ifstream file(fileName.cstr());
#endif
	if (!file)
		throw std::runtime_error("Could not load file " + fileName.str());

	OBJMaterial *currentMaterial = nullptr;

	std::string line;
	while (std::getline(file, line))
	{
		if (file.eof())
			break;
		if (line.size() == 0)
			continue;
		std::stringstream ss(line);
		std::string header;
		ss >> header;

		if (header == "newmtl")
		{
			std::string materialName;
			ss >> materialName;
			std::unique_ptr<OBJMaterial> material = std::make_unique<OBJMaterial>();
			currentMaterial = material.get();
			currentMaterial->name = materialName;
			materials.insert(std::make_pair(materialName, std::move(material)));
		}
		else 
		{
			if (header == "map_Kd") {
				std::string texturePath;
				ss >> texturePath;
				currentMaterial->map_Kd = texturePath;
			} else if (header == "map_Ka") {
				std::string texturePath;
				ss >> texturePath;
				currentMaterial->map_Ka = texturePath;
			} else if (header == "map_bump") {
				std::string texturePath;
				ss >> texturePath;
				currentMaterial->map_bump = texturePath;
			}
			else if (header == "map_Disp") {
				std::string texturePath;
				ss >> texturePath;
				currentMaterial->map_Disp = texturePath;
			} else if (header == "Ka") {
				ss >> currentMaterial->Ka.x;
				ss >> currentMaterial->Ka.y;
				ss >> currentMaterial->Ka.z;
			} else if (header == "Kd") {
				ss >> currentMaterial->Kd.x;
				ss >> currentMaterial->Kd.y;
				ss >> currentMaterial->Kd.z;
			} else if (header == "Ks") {
				ss >> currentMaterial->Ks.x;
				ss >> currentMaterial->Ks.y;
				ss >> currentMaterial->Ks.z;
			} else if (header == "Ke") {
				ss >> currentMaterial->Ke.x;
				ss >> currentMaterial->Ke.y;
				ss >> currentMaterial->Ke.z;
			} else if (header == "Ni") {
				ss >> currentMaterial->Ni;
			} else if (header == "Ns") {
				ss >> currentMaterial->Ns;
			} else if (header == "d") {
				ss >> currentMaterial->d;
			} else if (header == "illum") {
				ss >> currentMaterial->illum;
			}
		}
	}
}

Material::Ptr convert(const aka::Path &path, OBJMaterial *objMaterial)
{
	// TODO use others parameters.
	Texture::Ptr colorTexture;
	Texture::Ptr normalTexture;
	Sampler defaultSampler{};
	defaultSampler.filterMag = Sampler::Filter::Linear;
	defaultSampler.filterMin = Sampler::Filter::MipMapLinear;
	defaultSampler.wrapS = Sampler::Wrap::Repeat;
	defaultSampler.wrapT = Sampler::Wrap::Repeat;
	if (objMaterial->map_Kd.length() > 0)
	{
		aka::Image image = aka::Image::load(path + aka::Path(objMaterial->map_Kd));
		colorTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
		colorTexture->upload(image.bytes.data());
	}
	else if (objMaterial->map_Ka.length() > 0)
	{
		aka::Image image = aka::Image::load(path + aka::Path(objMaterial->map_Ka));
		colorTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
		colorTexture->upload(image.bytes.data());
	}
	else
	{
		colorTexture = Texture::create(1, 1, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
		uint8_t data[4] = { 255,255,255,255 };
		colorTexture->upload(data);
	}
	if (objMaterial->map_Disp.length() > 0)
	{
		aka::Image image = aka::Image::load(path + aka::Path(objMaterial->map_Disp));
		normalTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
		normalTexture->upload(image.bytes.data());
	}
	else
	{
		normalTexture = Texture::create(1, 1, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
		uint8_t data[4] = { 128,255,128,255 };
		normalTexture->upload(data);
	}
	Material::Ptr material = std::make_shared<Material>();
	material->doubleSided = false;
	material->color = color4f(1.f);
	material->metallic = 0.f;
	material->roughness = 0.f;
	material->colorTexture = colorTexture;
	material->normalTexture = normalTexture;
	material->metallicTexture = nullptr;
	material->emissiveTexture = nullptr;

	return material;
}

Model::Ptr OBJLoader::load(const Path& fileName)
{
	Model::Ptr model = std::make_shared<Model>();

	std::vector<point3f> positions;
	std::vector<norm3f> normals;
	std::vector<uv2f> uvs;
	std::vector<OBJObject> objects;
	std::map<std::string, Material::Ptr> materials;
	Material::Ptr currentMaterial = nullptr;

	aka::Path path = fileName.up();
#if defined(AKA_PLATFORM_WINDOWS)
	aka::StrWide wstr = aka::encoding::wide(fileName.str());
	std::ifstream file(wstr.cstr());
#else
	std::ifstream file(fileName.cstr());
#endif
	if (!file)
		throw std::runtime_error("Could not load file " + fileName.str());

	std::string line;
	while (std::getline(file, line))
	{
		if (file.eof())
			break;
		if (line.size() == 0)
			continue;
		std::stringstream ss(line);
		std::string header;
		ss >> header;
		if (header == "mtllib")
		{
			std::string materialFileName;
			ss >> materialFileName;
			std::map<std::string, std::unique_ptr<OBJMaterial>> parsedMaterials;
			parseMaterials(path + aka::Path(materialFileName), parsedMaterials);
			// convert OBJmaterials to materials.
			for (auto const& it : parsedMaterials) {
				Material::Ptr material = convert(path, it.second.get());
				materials.insert(std::make_pair(it.first, material));
				// TODO link to node
			}
			continue;
		}
		else if (header == "usemtl")
		{
			std::string materialName;
			ss >> materialName; 
			auto it = materials.find(materialName);
			if (it == materials.end())
				throw std::runtime_error("Material not found : " + materialName);
			currentMaterial = it->second;
			continue;
		}
		switch (header[0])
		{
		case 'v':
		case 'V': {
			switch (line[1])
			{
			case 'n':
			case 'N': {
				norm3f norm;
				ss >> norm.x >> norm.y >> norm.z;
				normals.push_back(norm);
				break;
			}
			case 't':
			case 'T': {
				uv2f uv;
				ss >> uv.u >> uv.v;
				uvs.push_back(uv);
				break;
			}
			case ' ': {
				point3f pos;
				ss >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
				break;
			}
			default:
				break;
			}
			break;
		}
		case 'f':
		case 'F': {
			// If no object, create default one.
			if (objects.size() == 0)
			{
				objects.emplace_back();
				objects.back().name = "default";
			}
			// If no group, create default one.
			if (objects.back().groups.size() == 0)
			{
				objects.back().groups.emplace_back();
				objects.back().groups.back().name = "default";
			}
			OBJGroup &group = objects.back().groups.back();
			group.faces.emplace_back();
			if (currentMaterial != nullptr)
			{
				group.materials.push_back(currentMaterial);
			}
			else
			{
				OBJMaterial m;
				m.Kd = vec3f(1.f);
				group.materials.push_back(convert("", &m));
				currentMaterial = group.materials.back();
			}
			OBJFace &face = group.faces.back();
			while (ss.peek() != std::char_traits<char>::eof())
			{
				face.vertices.emplace_back();
				OBJVertex &vertex = face.vertices.back();
				ss >> vertex.posID;
				if (ss.peek() == '/')
				{
					ss.ignore(1);
					if (ss.peek() == '/')
					{
						ss.ignore(1);
						ss >> vertex.normID;
					}
					else
					{
						ss >> vertex.uvID;
						if (ss.peek() == '/')
						{
							ss.ignore(1);
							ss >> vertex.normID;
						}
					}
				}
				skipWhitespace(ss);
			}
			break;
		}
		case '#':
			break;
		case 'O':
		case 'o': { // New object
			std::string objectName;
			ss >> objectName;
			objects.emplace_back();
			objects.back().name = objectName;
			break;
		}
		case 'S':
		case 's': { // Smoothing group
			break;
		}
		case 'G':
		case 'g': { // New face group
			std::string groupName;
			ss >> groupName;
			if (objects.size() == 0)
			{
				objects.emplace_back();
				objects.back().name = "default";
			}
			objects.back().groups.emplace_back();
			objects.back().groups.back().name = groupName;
			break;
		}
		default:
			aka::Logger::warn("Unknown data : ", line);
			break;
		}
	}
	size_t totalTriCount = 0;
	for (const OBJObject &object : objects)
	{		
		uint32_t iVert = 0;
		for (const OBJGroup &group : object.groups)
		{
			Mesh::Ptr mesh = Mesh::create();
			Material material = *group.materials[0]; // TODO check materials to avoid duplicate
			mat4f transform = mat4f::identity();
			model->meshes.push_back(mesh);
			model->materials.push_back(material);
			model->transforms.push_back(transform);
			std::vector<Vertex> vertices;
			// One node per group
			for (size_t iFace = 0; iFace < group.faces.size(); iFace++)
			{
				AKA_ASSERT(group.materials[0] == group.materials[iFace], "Invalid material");
				const OBJFace &face = group.faces[iFace];
				if (face.vertices.size() == 3)
				{
					// It's a triangle !
					vec3f AB(positions[face[1].posID - 1] - positions[face[0].posID - 1]);
					vec3f AC(positions[face[2].posID - 1] - positions[face[0].posID - 1]);
					norm3f normal(vec3f::normalize(vec3f::cross(AB, AC)));
					for (size_t iVert = 0; iVert < 3; iVert++)
					{
						vertices.emplace_back();
						Vertex& vertex = vertices.back();
						vertex.position = positions[face[iVert].posID - 1];
						if (normals.size() > 0)
							vertex.normal = normals[face[iVert].normID - 1];
						else
							vertex.normal = normal;
						if (uvs.size() > 0)
							vertex.uv = uvs[face[iVert].uvID - 1];
						else
							vertex.uv = uv2f(0.f);
						vertex.color = color4f(1.f);
						model->bbox.include(vertex.position);
					}
					iVert += 3;
					totalTriCount += 1;
				}
				else if (face.vertices.size() == 4)
				{
					// It's a quad !
					vec3f AB(positions[face[1].posID - 1] - positions[face[0].posID - 1]);
					vec3f AC(positions[face[2].posID - 1] - positions[face[0].posID - 1]);
					norm3f normal(vec3f::normalize(vec3f::cross(AB, AC)));
					for (size_t iVert = 0; iVert < 4; iVert++)
					{
						vertices.emplace_back();
						Vertex& vertex = vertices.back();
						vertex.position = positions[face[iVert].posID - 1];
						if (normals.size() > 0)
							vertex.normal = normals[face[iVert].normID - 1];
						else
							vertex.normal = normal;
						if (uvs.size() > 0)
							vertex.uv = uvs[face[iVert].uvID - 1];
						else
							vertex.uv = uv2f(0.f);
						vertex.color = color4f(1.f);
						model->bbox.include(vertex.position);
					}
					iVert += 4;
					totalTriCount += 2;
				}
				else
				{
					aka::Logger::error("Face type not supported, skipping : ", face.vertices.size());
				}
			}
			VertexData data;
			data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
			data.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
			data.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
			data.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });
			mesh->vertices(data, vertices.data(), vertices.size());
		}
	}
	return model;
}

}
