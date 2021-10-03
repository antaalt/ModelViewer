#include "Importer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

namespace app {

struct AssimpImporter {
	AssimpImporter(const Path& directory, const aiScene* scene, aka::World& world);

	void process();
	void processNode(Entity parent, aiNode* node);
	Entity processMesh(aiMesh* mesh);

	Texture::Ptr loadTexture(const Path& path, TextureFlag flags);
private:
	Path m_directory;
	const aiScene* m_assimpScene;
	aka::World& m_world;
private:
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
	uint8_t bytesMissingColor[4] = { 255, 0, 255, 255 };
	uint8_t bytesBlankColor[4] = { 255, 255, 255, 255 };
	uint8_t bytesNormal[4] = { 128,128,255,255 };
	uint8_t bytesRoughness[4] = { 255,255,255,255 };
	m_missingColorTexture = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::ShaderResource, bytesMissingColor);
	m_blankColorTexture = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::ShaderResource, bytesBlankColor);
	m_missingNormalTexture = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::ShaderResource, bytesNormal);
	m_missingRoughnessTexture = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::ShaderResource, bytesRoughness);
}

void AssimpImporter::process()
{
	Entity root = m_world.createEntity(OS::File::basename(m_directory));
	root.add<Transform3DComponent>(Transform3DComponent{ mat4f::identity() });
	root.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), mat4f::identity() });
	processNode(root, m_assimpScene->mRootNode);
}

void AssimpImporter::processNode(Entity parent, aiNode* node)
{
	mat4f transform = mat4f(
		col4f(node->mTransformation[0][0], node->mTransformation[1][0], node->mTransformation[2][0], node->mTransformation[3][0]),
		col4f(node->mTransformation[0][1], node->mTransformation[1][1], node->mTransformation[2][1], node->mTransformation[3][1]),
		col4f(node->mTransformation[0][2], node->mTransformation[1][2], node->mTransformation[2][2], node->mTransformation[3][2]),
		col4f(node->mTransformation[0][3], node->mTransformation[1][3], node->mTransformation[2][3], node->mTransformation[3][3])
	);
	mat4f inverseParentTransform;
	if (parent.valid())
	{
		mat4f parentTransform = parent.get<Transform3DComponent>().transform;
		transform = parentTransform * transform;
		inverseParentTransform = mat4f::inverse(parentTransform);
	}
	else
	{
		inverseParentTransform = mat4f::identity();
	}
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		Entity e = processMesh(m_assimpScene->mMeshes[node->mMeshes[i]]);
		e.add<Transform3DComponent>(Transform3DComponent{ transform });
		e.add<Hierarchy3DComponent>(Hierarchy3DComponent{ parent, inverseParentTransform });
	}
	if (node->mNumChildren > 0)
	{
		Entity entity = m_world.createEntity(node->mName.C_Str());
		entity.add<Hierarchy3DComponent>(Hierarchy3DComponent{ parent, inverseParentTransform });
		entity.add<Transform3DComponent>(Transform3DComponent{ transform });
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			processNode(entity, node->mChildren[i]);
	}
}

// TODO move to engine.
struct Vertex {
	point3f position;
	norm3f normal;
	uv2f uv;
	color4f color;
}; 

Entity AssimpImporter::processMesh(aiMesh* mesh)
{
	AKA_ASSERT(mesh->HasPositions(), "Mesh need positions");
	AKA_ASSERT(mesh->HasNormals(), "Mesh needs normals");

	String meshName = mesh->mName.C_Str();
	Entity e = m_world.createEntity(meshName);
	e.add<MeshComponent>();
	e.add<MaterialComponent>();
	MeshComponent& meshComponent = e.get<MeshComponent>();
	MaterialComponent& materialComponent = e.get<MaterialComponent>();

	if (!ResourceManager::has<Mesh>(meshName))
	{
		std::vector<Vertex> vertices(mesh->mNumVertices);
		std::vector<uint32_t> indices;
		// process vertices
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
		// Import resources
		Path bufferDirectory = "library/buffer/";
		if (!OS::Directory::exist(bufferDirectory))
			OS::Directory::create(bufferDirectory);
		Path meshDirectory = "library/mesh/";
		if (!OS::Directory::exist(bufferDirectory))
			OS::Directory::create(bufferDirectory);
		// Index buffer
		String indexBufferName = meshName + "-indices";
		String indexBufferFileName = indexBufferName + ".buffer";
		Path indexBufferPath = bufferDirectory + indexBufferFileName;
		{
			BufferStorage indexBuffer;
			indexBuffer.type = BufferType::Index;
			indexBuffer.access = BufferCPUAccess::None;
			indexBuffer.usage = BufferUsage::Immutable;
			indexBuffer.bytes.resize(indices.size() * sizeof(uint32_t));
			memcpy(indexBuffer.bytes.data(), indices.data(), indexBuffer.bytes.size());
			if (!indexBuffer.save(indexBufferPath))
				Logger::error("Failed to save buffer");
		}
		Buffer::Ptr indexBuffer = ResourceManager::load<Buffer>(indexBufferName, indexBufferPath).resource;
		
		// Vertex buffer
		String vertexBufferName = meshName + "-vertices";
		String vertexBufferFileName = vertexBufferName + ".buffer";
		Path vertexBufferPath = bufferDirectory + vertexBufferFileName;
		{
			BufferStorage vertexBuffer;
			vertexBuffer.type = BufferType::Vertex;
			vertexBuffer.access = BufferCPUAccess::None;
			vertexBuffer.usage = BufferUsage::Immutable;
			vertexBuffer.bytes.resize(vertices.size() * sizeof(Vertex));
			memcpy(vertexBuffer.bytes.data(), vertices.data(), vertexBuffer.bytes.size());
			if (!vertexBuffer.save(vertexBufferPath))
				Logger::error("Failed to save buffer");
		}
		Buffer::Ptr vertexBuffer = ResourceManager::load<Buffer>(vertexBufferName, vertexBufferPath).resource;

		// Mesh
		String meshFileName = meshName + ".mesh";
		Path meshPath = meshDirectory + meshFileName;
		{
			uint32_t vertexCount = (uint32_t)vertices.size();
			uint32_t vertexBufferSize = (uint32_t)(vertices.size() * sizeof(Vertex));
			MeshStorage storage;
			storage.vertices = { {
				MeshStorage::Vertex {
					VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, position), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::Vertex {
					VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, normal), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::Vertex {
					VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, uv), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::Vertex {
					VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, color), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				}
			} };
			storage.indexBufferName = indexBufferName;
			storage.indexBufferOffset = 0;
			storage.indexCount = (uint32_t)indices.size();
			storage.indexFormat = IndexFormat::UnsignedInt;
			if (!storage.save(meshPath))
				Logger::error("Failed to save mesh");
			ResourceManager::load<Mesh>(meshName, meshPath);
		}
	}

	meshComponent.submesh.mesh = ResourceManager::get<Mesh>(meshName);
	meshComponent.submesh.type = PrimitiveType::Triangles;
	meshComponent.submesh.count = meshComponent.submesh.mesh->getIndexCount();
	meshComponent.submesh.offset = 0;
	
	// process material
	if (mesh->mMaterialIndex >= 0)
	{
		//aiTextureType_EMISSION_COLOR = 14,
		//aiTextureType_METALNESS = 15,
		//aiTextureType_DIFFUSE_ROUGHNESS = 16,
		//aiTextureType_AMBIENT_OCCLUSION = 17,
		aiMaterial* material = m_assimpScene->mMaterials[mesh->mMaterialIndex];
		TextureSampler defaultSampler = TextureSampler::trilinear;
		TextureFlag flags = TextureFlag::ShaderResource | TextureFlag::GenerateMips;
		aiColor4D c;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, c);
		material->Get(AI_MATKEY_TWOSIDED, materialComponent.doubleSided);
		materialComponent.color = color4f(c.r, c.g, c.b, c.a);
		if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
		{
			aiTextureType type = aiTextureType_BASE_COLOR;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.albedo.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.albedo.sampler = defaultSampler;
				if (materialComponent.albedo.texture == nullptr)
					materialComponent.albedo.texture = m_missingColorTexture;
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
				materialComponent.albedo.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.albedo.sampler = defaultSampler;
				if (materialComponent.albedo.texture == nullptr)
					materialComponent.albedo.texture = m_missingColorTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.albedo.texture = m_blankColorTexture;
			materialComponent.albedo.sampler = defaultSampler;
		}
		if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
		{
			aiTextureType type = aiTextureType_NORMAL_CAMERA;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.normal.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.normal.sampler = defaultSampler;
				if (materialComponent.normal.texture == nullptr)
					materialComponent.normal.texture = m_missingNormalTexture;
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
				materialComponent.normal.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.normal.sampler = defaultSampler;
				if (materialComponent.normal.texture == nullptr)
					materialComponent.normal.texture = m_missingNormalTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.normal.texture = m_missingNormalTexture;
			materialComponent.normal.sampler = defaultSampler;
		}

		if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) // GLTF pbr texture is retrieved this way (?)
		{
			aiTextureType type = aiTextureType_UNKNOWN;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, 0, &str);
				materialComponent.material.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.material.sampler = defaultSampler;
				if (materialComponent.material.texture == nullptr)
					materialComponent.material.texture = m_missingRoughnessTexture;
				break; // Ignore others textures for now.
			}
		}
		else if (material->GetTextureCount(aiTextureType_SHININESS) > 0)
		{
			aiTextureType type = aiTextureType_SHININESS;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.material.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				materialComponent.material.sampler = defaultSampler;
				if (materialComponent.material.texture == nullptr)
					materialComponent.material.texture = m_missingRoughnessTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.material.texture = m_missingRoughnessTexture;
			materialComponent.material.sampler = defaultSampler;
		}
	}
	else
	{
		// No material !
		materialComponent.color = color4f(1.f);
		materialComponent.doubleSided = true;
		materialComponent.albedo.texture = m_blankColorTexture;
		materialComponent.albedo.sampler = TextureSampler::nearest;
		materialComponent.normal.texture = m_missingNormalTexture;
		materialComponent.normal.sampler = TextureSampler::nearest;
		materialComponent.material.texture = m_missingRoughnessTexture;
		materialComponent.material.sampler = TextureSampler::nearest;
	}
	return e;
}

Texture::Ptr AssimpImporter::loadTexture(const Path& path, TextureFlag flags)
{
	String name = OS::File::name(path);
	if (ResourceManager::has<Texture>(name))
		return ResourceManager::get<Texture>(name);
	if (Importer::importTexture2D(name, path, flags))
	{
		return ResourceManager::get<Texture>(name);
	}
	else
	{
		Logger::error("Failed to import texture2D");
		return nullptr;
	}
}

bool Importer::importScene(const Path& path, aka::World& world)
{
	Assimp::Importer assimpImporter;
	const aiScene* aiScene = assimpImporter.ReadFile(path.cstr(),
		aiProcess_Triangulate |
		//aiProcess_CalcTangentSpace |
#if defined(AKA_ORIGIN_TOP_LEFT)
		aiProcess_FlipUVs |
#endif
#if defined(GEOMETRY_LEFT_HANDED)
		aiProcess_MakeLeftHanded |
#endif
		aiProcess_GenSmoothNormals
	);
	if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode)
	{
		Logger::error("[assimp] ", assimpImporter.GetErrorString());
		return false;
	}
	Path directory = path.up();
	AssimpImporter importer(directory, aiScene, world);
	importer.process();
	return true;
}

bool Importer::importMesh(const aka::String& name, const aka::Path& path)
{
	return false;
}

bool Importer::importTexture2D(const aka::String& name, const aka::Path& path, TextureFlag flags)
{
	// TODO use devil as importer to support a wider range of format ?
	// Exporter would be a standalone exe to not overwhelm the engine with assimp, devil include...
	// Or a library that include aka, assimp, devil...
	// Use it as library for a project. 
	// AkaImporter.h
	if (!ResourceManager::has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = TextureType::Texture2D;
		storage.flags = flags;
		storage.format = TextureFormat::RGBA8;
		storage.images.push_back(Image::load(path));

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (ResourceManager::load<Texture>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

bool Importer::importTexture2DHDR(const aka::String& name, const aka::Path& path, TextureFlag flags)
{
	if (!ResourceManager::has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = TextureType::Texture2D;
		storage.flags = flags;
		storage.format = TextureFormat::RGBA32F;
		storage.images.push_back(Image::loadHDR(path));

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (ResourceManager::load<Texture>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

bool Importer::importTextureCubemap(const aka::String& name, const aka::Path& px, const aka::Path& py, const aka::Path& pz, const aka::Path& nx, const aka::Path& ny, const aka::Path& nz, TextureFlag flags)
{
	if (!ResourceManager::has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = TextureType::TextureCubeMap;
		storage.flags = flags;
		storage.format = TextureFormat::RGBA8;
		storage.images.push_back(Image::load(px));
		storage.images.push_back(Image::load(py));
		storage.images.push_back(Image::load(pz));
		storage.images.push_back(Image::load(nx));
		storage.images.push_back(Image::load(ny));
		storage.images.push_back(Image::load(nz));

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (ResourceManager::load<Texture>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

bool Importer::importAudio(const aka::String& name, const aka::Path& path)
{
	String libPath = "library/audio/" + name + ".audio";
	// Convert and save (only copy for now)
	if (!OS::File::create(libPath))
		Logger::error("Failed to create file");
	if (!OS::File::copy(path, libPath))
		Logger::error("Failed to copy file");
	// Load
	ResourceManager::load<AudioStream>(name, libPath);
	return true;
}

bool Importer::importFont(const aka::String& name, const aka::Path& path)
{
	if (!ResourceManager::has<Font>(name))
	{
		String directory = "library/font/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".font";
		Blob blob;
		if (!OS::File::read(path, &blob))
			return false;
		// Convert and save
		FontStorage storage;
		storage.ttf = std::vector<byte_t>(blob.data(), blob.data() + blob.size());

		if (!storage.save(libPath))
			return false;
		// Load
		if (ResourceManager::load<Font>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Font already imported : ", name);
	}
	return true;
}

};