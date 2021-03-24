#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

namespace viewer {

Node processMesh(const Path& path, aiMesh* mesh, const aiScene* scene, const mat4f& transform, aabbox<>& bbox)
{
	std::vector<Vertex> vertices(mesh->mNumVertices);
	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex& vertex = vertices[i];
		// process vertex positions, normals and texture coordinates
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;
		bbox.include(transform.multiplyPoint(vertex.position));

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			vertex.uv.u = mesh->mTextureCoords[0][i].x;
			vertex.uv.v = mesh->mTextureCoords[0][i].y;
		}
		else
			vertex.uv = uv2f(0.f);
		if (mesh->mColors[0])
		{
			vertex.color.r = mesh->mColors[0][i].r;
			vertex.color.g = mesh->mColors[0][i].g;
			vertex.color.b = mesh->mColors[0][i].b;
			vertex.color.a = mesh->mColors[0][i].a;
		}
		else
			vertex.color = color4f(1.f);
	}
	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	Node node;
	// process material
	if (mesh->mMaterialIndex >= 0)
	{
		//aiTextureType_EMISSION_COLOR = 14,
		//aiTextureType_METALNESS = 15,
		//aiTextureType_DIFFUSE_ROUGHNESS = 16,
		//aiTextureType_AMBIENT_OCCLUSION = 17,
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		Sampler defaultSampler{};
		defaultSampler.filterMag = Sampler::Filter::Linear;
		defaultSampler.filterMin = Sampler::Filter::MipMapLinear;
		defaultSampler.wrapS = Sampler::Wrap::Repeat;
		defaultSampler.wrapT = Sampler::Wrap::Repeat;
		node.material.color = color4f(1.f);
		node.material.doubleSided = true;
		node.material.metallic = 1.f;
		node.material.roughness = 1.f;
		if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
		{
			aiTextureType type = aiTextureType_BASE_COLOR;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				Image image = Image::load(Path(path + str.C_Str()));
				node.material.colorTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
				node.material.colorTexture->upload(image.bytes.data());
				break; // Ignore others textures for now.
			}
		}
		else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiTextureType type = aiTextureType_DIFFUSE;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				Image image = Image::load(Path(path + str.C_Str()));
				node.material.colorTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
				node.material.colorTexture->upload(image.bytes.data());
				break; // Ignore others textures for now.
			}
		}
		else
		{
			// TODO cache this
			uint8_t data[4] = { 255,255,255,255 };
			node.material.colorTexture = Texture::create(1, 1, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
			node.material.colorTexture->upload(data);
		}
		if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
		{
			aiTextureType type = aiTextureType_NORMAL_CAMERA;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				Image image = Image::load(Path(path + str.C_Str()));
				node.material.normalTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
				node.material.normalTexture->upload(image.bytes.data());
				break; // Ignore others textures for now.
			}
		}
		if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
		{
			aiTextureType type = aiTextureType_NORMALS;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				Image image = Image::load(Path(path + str.C_Str()));
				node.material.normalTexture = Texture::create(image.width, image.height, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
				node.material.normalTexture->upload(image.bytes.data());
				break; // Ignore others textures for now.
			}
		}
		else
		{
			// TODO cache this
			uint8_t data[4] = { 128,255,128,255 };
			node.material.normalTexture = Texture::create(1, 1, Texture::Format::UnsignedByte, Texture::Component::RGBA, defaultSampler);
			node.material.normalTexture->upload(data);
		}
	}
	else
	{
		// No material !
	}
	node.mesh = Mesh::create();
	VertexData data;
	data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
	data.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });
	node.mesh->vertices(data, vertices.data(), vertices.size());
	node.mesh->indices(IndexFormat::UnsignedInt, indices.data(), indices.size());
	node.transform = transform;
	return node;
}

void processNode(const Path& path, Model::Ptr model, aiNode* node, const aiScene* scene)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mat4f transform = mat4f(
			col4f(node->mTransformation[0][0], node->mTransformation[1][0], node->mTransformation[2][0], node->mTransformation[3][0]),
			col4f(node->mTransformation[0][1], node->mTransformation[1][1], node->mTransformation[2][1], node->mTransformation[3][1]),
			col4f(node->mTransformation[0][2], node->mTransformation[1][2], node->mTransformation[2][2], node->mTransformation[3][2]),
			col4f(node->mTransformation[0][3], node->mTransformation[1][3], node->mTransformation[2][3], node->mTransformation[3][3])
		);

		model->nodes.push_back(processMesh(path, mesh, scene, transform, model->bbox));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(path, model, node->mChildren[i], scene);
	}
}

Model::Ptr ModelLoader::load(const Path& path, const point3f& origin, float scale)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.cstr(), 
		aiProcess_Triangulate | 
		//aiProcess_CalcTangentSpace |
		//aiProcess_FlipUVs |
		aiProcess_GenSmoothNormals
	);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		Logger::error("[assimp] ", importer.GetErrorString());
		return nullptr;
	}
	String directory = path.str().substr(0, path.str().findLast('/'));

	Model::Ptr model = std::make_shared<Model>();
	processNode(directory, model, scene->mRootNode, scene);
	return model;
}

};