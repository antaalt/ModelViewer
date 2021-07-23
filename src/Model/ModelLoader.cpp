#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

namespace viewer {

struct AssimpImporter {
	AssimpImporter(const Path& directory, const aiScene* scene, aka::World& world);

	void process();
	void processNode(const mat4f& transform, aiNode* node);
	void processMesh(const mat4f& transform, aiMesh* mesh);

	Texture::Ptr loadTexture(const Path& path, const Sampler& sampler);
private:
	Path m_directory;
	const aiScene* m_assimpScene;
	aka::World& m_world;
private:
	std::map<String, Texture::Ptr> m_textureCache;
	Texture::Ptr m_missingColorTexture;
	Texture::Ptr m_blankColorTexture;
	Texture::Ptr m_missingNormalTexture;
	Texture::Ptr m_missingRoughnessTexture;
};

AssimpImporter::AssimpImporter(const Path& directory, const aiScene* scene, aka::World& world) :
	m_directory(directory),
	m_assimpScene(scene),
	m_world(world)
{
	Sampler defaultSampler = Sampler::trilinear();
	uint8_t bytesMissingColor[4] = { 255, 0, 255, 255 };
	uint8_t bytesBlankColor[4] = { 255, 255, 255, 255 };
	uint8_t bytesNormal[4] = { 128,128,255,255 };
	uint8_t bytesRoughness[4] = { 255,255,255,255 };
	m_missingColorTexture = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, defaultSampler, bytesMissingColor);
	m_blankColorTexture = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, defaultSampler, bytesBlankColor);
	m_missingNormalTexture = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, defaultSampler, bytesNormal);
	m_missingRoughnessTexture = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, defaultSampler, bytesRoughness);
}

void AssimpImporter::process()
{
	processNode(mat4f::identity(), m_assimpScene->mRootNode);
}

void AssimpImporter::processNode(const mat4f& transform, aiNode* node)
{
	mat4f t = transform * mat4f(
		col4f(node->mTransformation[0][0], node->mTransformation[1][0], node->mTransformation[2][0], node->mTransformation[3][0]),
		col4f(node->mTransformation[0][1], node->mTransformation[1][1], node->mTransformation[2][1], node->mTransformation[3][1]),
		col4f(node->mTransformation[0][2], node->mTransformation[1][2], node->mTransformation[2][2], node->mTransformation[3][2]),
		col4f(node->mTransformation[0][3], node->mTransformation[1][3], node->mTransformation[2][3], node->mTransformation[3][3])
	);
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		processMesh(t, m_assimpScene->mMeshes[node->mMeshes[i]]);
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(t, node->mChildren[i]);
	}
}

struct Vertex {
	point3f position;
	norm3f normal;
	uv2f uv;
	color4f color;
};

void AssimpImporter::processMesh(const mat4f& transform, aiMesh* mesh)
{
	std::vector<Vertex> vertices(mesh->mNumVertices);
	std::vector<unsigned int> indices;

	AKA_ASSERT(mesh->HasPositions(), "Mesh need positions");
	AKA_ASSERT(mesh->HasNormals(), "Mesh needs normals");

	Entity e = m_world.createEntity(mesh->mName.C_Str());
	e.add<Transform3DComponent>();
	e.add<MeshComponent>();
	e.add<MaterialComponent>();
	Transform3DComponent& transformComponent = e.get<Transform3DComponent>();
	MeshComponent& meshComponent = e.get<MeshComponent>();
	MaterialComponent& materialComponent = e.get<MaterialComponent>();

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex& vertex = vertices[i];
		// process vertex positions, normals and texture coordinates
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;
		meshComponent.bounds.include(vertex.position);

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;
		if (mesh->HasTextureCoords(0))
		{
			vertex.uv.u = mesh->mTextureCoords[0][i].x;
			vertex.uv.v = mesh->mTextureCoords[0][i].y;
		}
		else
			vertex.uv = uv2f(0.f);
		if (mesh->HasVertexColors(0))
		{
			vertex.color.r = mesh->mColors[0][i].r;
			vertex.color.g = mesh->mColors[0][i].g;
			vertex.color.b = mesh->mColors[0][i].b;
			vertex.color.a = mesh->mColors[0][i].a;
		}
		else
			vertex.color = color4f(1.f);
		mesh->mTangents;
		mesh->mBitangents;
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// process material
	if (mesh->mMaterialIndex >= 0)
	{
		//aiTextureType_EMISSION_COLOR = 14,
		//aiTextureType_METALNESS = 15,
		//aiTextureType_DIFFUSE_ROUGHNESS = 16,
		//aiTextureType_AMBIENT_OCCLUSION = 17,
		aiMaterial* material = m_assimpScene->mMaterials[mesh->mMaterialIndex];
		Sampler defaultSampler = Sampler::trilinear();
		aiColor4D c;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, c);
		material->Get(AI_MATKEY_TWOSIDED, materialComponent.doubleSided);
		materialComponent.color = color4f(c.r, c.g, c.b, c.a);
		// TODO create unordered_map to avoid duplicating textures on GPU
		if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
		{
			aiTextureType type = aiTextureType_BASE_COLOR;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.colorTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.colorTexture == nullptr)
					materialComponent.colorTexture = m_missingColorTexture;
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
				materialComponent.colorTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.colorTexture == nullptr)
					materialComponent.colorTexture = m_missingColorTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.colorTexture = m_blankColorTexture;
		}
		if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
		{
			aiTextureType type = aiTextureType_NORMAL_CAMERA;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.normalTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.normalTexture == nullptr)
					materialComponent.normalTexture = m_missingNormalTexture;
				break; // Ignore others textures for now.
			}
		}
		else if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
		{
			aiTextureType type = aiTextureType_NORMALS;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.normalTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.normalTexture == nullptr)
					materialComponent.normalTexture = m_missingNormalTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.normalTexture = m_missingNormalTexture;
		}

		if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) // GLTF pbr texture is retrieved this way (?)
		{
			aiTextureType type = aiTextureType_UNKNOWN;
			//for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, 0, &str);
				materialComponent.roughnessTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.roughnessTexture == nullptr)
					materialComponent.roughnessTexture = m_missingRoughnessTexture;
				//break; // Ignore others textures for now.
			}
		}
		else if (material->GetTextureCount(aiTextureType_SHININESS) > 0)
		{
			aiTextureType type = aiTextureType_SHININESS;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.roughnessTexture = loadTexture(Path(m_directory + str.C_Str()), defaultSampler);
				if (materialComponent.roughnessTexture == nullptr)
					materialComponent.roughnessTexture = m_missingRoughnessTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.roughnessTexture = m_missingRoughnessTexture;
		}
	}
	else
	{
		// No material !
		materialComponent.color = color4f(1.f);
		materialComponent.doubleSided = true;
		materialComponent.colorTexture = m_blankColorTexture;
		materialComponent.normalTexture = m_missingNormalTexture;
		materialComponent.roughnessTexture = m_missingRoughnessTexture;
	}
	VertexData data;
	data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
	data.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });
	meshComponent.submesh.mesh = Mesh::create();
	meshComponent.submesh.mesh->vertices(data, vertices.data(), vertices.size());
	meshComponent.submesh.mesh->indices(IndexFormat::UnsignedInt, indices.data(), indices.size());
	meshComponent.submesh.type = PrimitiveType::Triangles;
	meshComponent.submesh.indexCount = static_cast<uint32_t>(indices.size()); // TODO 0 means all
	meshComponent.submesh.indexOffset = 0;
	transformComponent.transform = transform;
}

Texture::Ptr AssimpImporter::loadTexture(const Path& path, const Sampler& sampler)
{
	auto it = m_textureCache.find(path.str());
	if (it == m_textureCache.end())
	{
		Image img = Image::load(path);
		if (img.bytes.size() > 0)
		{
			Texture::Ptr texture = Texture::create2D(
				img.width,
				img.height,
				TextureFormat::UnsignedByte,
				img.components == 4 ? TextureComponent::RGBA : TextureComponent::RGB,
				TextureFlag::None,
				sampler,
				img.bytes.data()
			);
			m_textureCache.insert(std::make_pair(path.str(), texture));
			return texture;
		}
		else
			return nullptr;
	}
	else
	{
		return it->second;
	}
}

bool ModelLoader::load(const Path& path, aka::World& world)
{
	Assimp::Importer assimpImporter;
	const aiScene* aiScene = assimpImporter.ReadFile(path.cstr(),
		aiProcess_Triangulate | 
		//aiProcess_CalcTangentSpace |
		//aiProcess_FlipUVs |
		aiProcess_GenSmoothNormals
	);
	if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode)
	{
		Logger::error("[assimp] ", assimpImporter.GetErrorString());
		return false;
	}
	String directory = path.str().substr(0, path.str().findLast('/'));
	AssimpImporter importer(directory, aiScene, world);
	importer.process();
	return true;
}

};