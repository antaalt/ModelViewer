#include "Importer.h"

#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

namespace app {

struct AssimpImporter {
	AssimpImporter(const Path& directory, const aiScene* scene, aka::World& world);

	void process();
	void processNode(Entity parent, aiNode* node);
	Entity processMesh(aiMesh* mesh);

	gfx::TextureHandle loadTexture(const Path& path, gfx::TextureFlag flags);
private:
	Path m_directory;
	const aiScene* m_assimpScene;
	aka::World& m_world;
private:
	gfx::TextureHandle m_missingColorTexture;
	gfx::TextureHandle m_blankColorTexture;
	gfx::TextureHandle m_missingNormalTexture;
	gfx::TextureHandle m_missingRoughnessTexture;
};

static const char* DefaultBlankColorTexture = "Aka.Default.BlankColorTexture";
static const char* DefaultMissingColorTexture = "Aka.Default.MissingColorTexture";
static const char* DefaultMissingNormalTexture = "Aka.Default.MissingNormalTexture";
static const char* DefaultMissingMaterialTexture = "Aka.Default.MissingMaterialTexture";

AssimpImporter::AssimpImporter(const Path& directory, const aiScene* scene, aka::World& world) :
	m_directory(directory),
	m_assimpScene(scene),
	m_world(world)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	// Store default texture in resource storage
	if (!resource->has<Texture>(DefaultBlankColorTexture))
	{
		uint8_t bytesBlankColor[4] = { 255, 255, 255, 255 };
		resource->load<Texture>(DefaultBlankColorTexture, new Texture{ gfx::Texture::create2D(1, 1, gfx::TextureFormat::RGBA8, gfx::TextureFlag::ShaderResource, bytesBlankColor) });
	}
	if (!resource->has<Texture>(DefaultMissingColorTexture))
	{
		uint8_t bytesMissingColor[4] = { 255, 0, 255, 255 };
		resource->load<Texture>(DefaultMissingColorTexture, new Texture{ gfx::Texture::create2D(1, 1, gfx::TextureFormat::RGBA8, gfx::TextureFlag::ShaderResource, bytesMissingColor) });
	}
	if (!resource->has<Texture>(DefaultMissingNormalTexture))
	{
		uint8_t bytesNormal[4] = { 128,128,255,255 };
		resource->load<Texture>(DefaultMissingNormalTexture, new Texture{ gfx::Texture::create2D(1, 1, gfx::TextureFormat::RGBA8, gfx::TextureFlag::ShaderResource, bytesNormal) });
	}
	if (!resource->has<Texture>(DefaultMissingMaterialTexture))
	{
		uint8_t bytesRoughness[4] = { 255,255,255,255 };
		resource->load<Texture>(DefaultMissingMaterialTexture, new Texture{ gfx::Texture::create2D(1, 1, gfx::TextureFormat::RGBA8, gfx::TextureFlag::ShaderResource, bytesRoughness) });
	}
	m_missingColorTexture = resource->get<Texture>(DefaultMissingColorTexture)->texture;
	m_blankColorTexture = resource->get<Texture>(DefaultMissingColorTexture)->texture;
	m_missingNormalTexture = resource->get<Texture>(DefaultMissingColorTexture)->texture;
	m_missingRoughnessTexture = resource->get<Texture>(DefaultMissingColorTexture)->texture;
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

	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	gfx::GraphicDevice* device = app->graphic();
	String meshName = mesh->mName.C_Str();
	Entity e = m_world.createEntity(meshName);
	e.add<MeshComponent>();
	e.add<MaterialComponent>();
	MeshComponent& meshComponent = e.get<MeshComponent>();
	MaterialComponent& materialComponent = e.get<MaterialComponent>();

	if (!resource->has<Mesh>(meshName))
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
		if (!OS::Directory::exist(meshDirectory))
			OS::Directory::create(meshDirectory);
		// Index buffer
		String indexBufferName = meshName + "-indices";
		String indexBufferFileName = indexBufferName + ".buffer";
		Path indexBufferPath = bufferDirectory + indexBufferFileName;
		{
			BufferStorage indexBuffer;
			indexBuffer.type = gfx::BufferType::Index;
			indexBuffer.access = gfx::BufferCPUAccess::None;
			indexBuffer.usage = gfx::BufferUsage::Immutable;
			indexBuffer.bytes.resize(indices.size() * sizeof(uint32_t));
			memcpy(indexBuffer.bytes.data(), indices.data(), indexBuffer.bytes.size());
			if (!indexBuffer.save(indexBufferPath))
				Logger::error("Failed to save buffer");
		}
		gfx::BufferHandle indexBuffer = resource->load<Buffer>(indexBufferName, indexBufferPath).resource.get()->buffer;
		
		// Vertex buffer
		String vertexBufferName = meshName + "-vertices";
		String vertexBufferFileName = vertexBufferName + ".buffer";
		Path vertexBufferPath = bufferDirectory + vertexBufferFileName;
		{
			BufferStorage vertexBuffer;
			vertexBuffer.type = gfx::BufferType::Vertex;
			vertexBuffer.access = gfx::BufferCPUAccess::None;
			vertexBuffer.usage = gfx::BufferUsage::Immutable;
			vertexBuffer.bytes.resize(vertices.size() * sizeof(Vertex));
			memcpy(vertexBuffer.bytes.data(), vertices.data(), vertexBuffer.bytes.size());
			if (!vertexBuffer.save(vertexBufferPath))
				Logger::error("Failed to save buffer");
		}
		gfx::BufferHandle vertexBuffer = resource->load<Buffer>(vertexBufferName, vertexBufferPath).resource.get()->buffer;

		// Mesh
		String meshFileName = meshName + ".mesh";
		Path meshPath = meshDirectory + meshFileName;
		{
			uint32_t vertexCount = (uint32_t)vertices.size();
			uint32_t vertexBufferSize = (uint32_t)(vertices.size() * sizeof(Vertex));
			MeshStorage storage;
			storage.vertices = { {
				MeshStorage::VertexBinding {
					gfx::VertexAttribute{ gfx::VertexSemantic::Position, gfx::VertexFormat::Float, gfx::VertexType::Vec3 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, position), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::VertexBinding {
					gfx::VertexAttribute{ gfx::VertexSemantic::Normal, gfx::VertexFormat::Float, gfx::VertexType::Vec3 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, normal), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::VertexBinding {
					gfx::VertexAttribute{ gfx::VertexSemantic::TexCoord0, gfx::VertexFormat::Float, gfx::VertexType::Vec2 },
					vertexBufferName,
					vertexCount, // count
					offsetof(Vertex, uv), // offset
					0,
					vertexBufferSize, // size
					sizeof(Vertex), // stride
				},
				MeshStorage::VertexBinding {
					gfx::VertexAttribute{ gfx::VertexSemantic::Color0, gfx::VertexFormat::Float, gfx::VertexType::Vec4 },
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
			storage.indexFormat = gfx::IndexFormat::UnsignedInt;
			if (!storage.save(meshPath))
				Logger::error("Failed to save mesh");
			resource->load<Mesh>(meshName, meshPath);
		}
	}

	meshComponent.mesh = resource->get<Mesh>(meshName);
	
	// process material
	if (mesh->mMaterialIndex >= 0)
	{
		//aiTextureType_EMISSION_COLOR = 14,
		//aiTextureType_METALNESS = 15,
		//aiTextureType_DIFFUSE_ROUGHNESS = 16,
		//aiTextureType_AMBIENT_OCCLUSION = 17,
		aiMaterial* material = m_assimpScene->mMaterials[mesh->mMaterialIndex];
		gfx::TextureFlag flags = gfx::TextureFlag::ShaderResource | gfx::TextureFlag::GenerateMips;
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
				materialComponent.albedo.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.albedo.texture.texture.data == nullptr)
					materialComponent.albedo.texture.texture = m_missingColorTexture;
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
				materialComponent.albedo.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.albedo.texture.texture.data == nullptr)
					materialComponent.albedo.texture.texture = m_missingColorTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.albedo.texture.texture = m_blankColorTexture;
		}
		materialComponent.albedo.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.albedo.texture.texture.data->width, materialComponent.albedo.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);
		if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
		{
			aiTextureType type = aiTextureType_NORMAL_CAMERA;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, i, &str);
				materialComponent.normal.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.normal.texture.texture.data == nullptr)
					materialComponent.normal.texture.texture = m_missingNormalTexture;
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
				materialComponent.normal.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.normal.texture.texture.data == nullptr)
					materialComponent.normal.texture.texture = m_missingNormalTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.normal.texture.texture = m_missingNormalTexture;
		}
		materialComponent.normal.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.normal.texture.texture.data->width, materialComponent.normal.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);

		if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) // GLTF pbr texture is retrieved this way (?)
		{
			aiTextureType type = aiTextureType_UNKNOWN;
			for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
			{
				aiString str;
				material->GetTexture(type, 0, &str);
				materialComponent.material.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.material.texture.texture.data == nullptr)
					materialComponent.material.texture.texture = m_missingRoughnessTexture;
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
				materialComponent.material.texture.texture = loadTexture(Path(m_directory + str.C_Str()), flags);
				if (materialComponent.material.texture.texture.data == nullptr)
					materialComponent.material.texture.texture = m_missingRoughnessTexture;
				break; // Ignore others textures for now.
			}
		}
		else
		{
			materialComponent.material.texture.texture = m_missingRoughnessTexture;
		}
		materialComponent.material.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.material.texture.texture.data->width, materialComponent.material.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);
	}
	else
	{
		// No material !
		materialComponent.color = color4f(1.f);
		materialComponent.doubleSided = true;
		materialComponent.albedo.texture.texture = m_blankColorTexture;
		materialComponent.albedo.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.albedo.texture.texture.data->width, materialComponent.albedo.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);
		materialComponent.normal.texture.texture = m_missingNormalTexture;
		materialComponent.normal.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.normal.texture.texture.data->width, materialComponent.normal.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);
		materialComponent.material.texture.texture = m_missingRoughnessTexture;
		materialComponent.material.sampler = device->createSampler(
			gfx::Filter::Linear, gfx::Filter::Linear,
			gfx::SamplerMipMapMode::Linear,
			gfx::Sampler::mipLevelCount(materialComponent.material.texture.texture.data->width, materialComponent.material.texture.texture.data->height),
			gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat, gfx::SamplerAddressMode::Repeat,
			1.f
		);
	}
	return e;
}

gfx::TextureHandle AssimpImporter::loadTexture(const Path& path, gfx::TextureFlag flags)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	String name = OS::File::name(path);
	if (resource->has<Texture>(name))
		return resource->get<Texture>(name)->texture;
	if (Importer::importTexture2D(name, path, flags))
	{
		return resource->get<Texture>(name)->texture;
	}
	else
	{
		Logger::error("Failed to import texture2D");
		return gfx::TextureHandle{};
	}
}

template <Assimp::Logger::ErrorSeverity Severity>
class LogStream : public Assimp::LogStream
{
public:
	LogStream() {}

	virtual void write(const char* message) override
	{
		// TODO better parsing
		std::string str(message);
		if (str.size() == 0)
			return;
		str.pop_back();
		switch (Severity)
		{
		case Assimp::Logger::Debugging:
			Logger::debug("[assimp]", str);
			break;
		case Assimp::Logger::Info:
			Logger::info("[assimp]", str);
			break;
		case Assimp::Logger::Warn:
			Logger::warn("[assimp]", str);
			break;
		case Assimp::Logger::Err:
			Logger::error("[assimp]", str);
			break;
		default:
			break;
		}
	}
};
using DebugLogStream = LogStream<Assimp::Logger::Debugging>;
using InfoLogStream = LogStream<Assimp::Logger::Info>;
using WarnLogStream = LogStream<Assimp::Logger::Warn>;
using ErrorLogStream = LogStream<Assimp::Logger::Err>;

Assimp::Logger* createAssimpLogger()
{
#if defined(AKA_DEBUG)
	Assimp::Logger::LogSeverity severity = Assimp::Logger::LogSeverity::VERBOSE;
#else
	Assimp::Logger::LogSeverity severity = Assimp::Logger::LogSeverity::NORMAL;
#endif
	Assimp::Logger* logger = Assimp::DefaultLogger::create("", severity, 0);
	logger->attachStream(new DebugLogStream(), Assimp::Logger::Debugging);
	logger->attachStream(new InfoLogStream(), Assimp::Logger::Info);
	logger->attachStream(new WarnLogStream(), Assimp::Logger::Warn);
	logger->attachStream(new ErrorLogStream(), Assimp::Logger::Err);
	return logger;
}

bool Importer::importScene(const Path& path, aka::World& world)
{
	createAssimpLogger();
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
	Assimp::DefaultLogger::kill();
	return true;
}

bool Importer::importMesh(const aka::String& name, const aka::Path& path)
{
	return false;
}

bool Importer::importTexture2D(const aka::String& name, const aka::Path& path, gfx::TextureFlag flags)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	// TODO use devil as importer to support a wider range of format ?
	// Exporter would be a standalone exe to not overwhelm the engine with assimp, devil include...
	// Or a library that include aka, assimp, devil...
	// Use it as library for a project. 
	// AkaImporter.h
	if (!resource->has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = gfx::TextureType::Texture2D;
		storage.flags = flags;
		storage.format = gfx::TextureFormat::RGBA8;
		storage.images.push_back(Image::load(path));
		if (has(flags, gfx::TextureFlag::GenerateMips))
			storage.levels = gfx::Sampler::mipLevelCount(storage.images[0].width(), storage.images[0].height());

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (resource->load<Texture>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

bool Importer::importTexture2DHDR(const aka::String& name, const aka::Path& path, gfx::TextureFlag flags)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	if (!resource->has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = gfx::TextureType::Texture2D;
		storage.flags = flags;
		storage.format = gfx::TextureFormat::RGBA32F;
		storage.images.push_back(Image::loadHDR(path));
		if (has(flags, gfx::TextureFlag::GenerateMips))
			storage.levels = gfx::Sampler::mipLevelCount(storage.images[0].width(), storage.images[0].height());

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (resource->load<Texture>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

bool Importer::importTextureCubemap(const aka::String& name, const aka::Path& px, const aka::Path& py, const aka::Path& pz, const aka::Path& nx, const aka::Path& ny, const aka::Path& nz, gfx::TextureFlag flags)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	if (!resource->has<Texture>(name))
	{
		String directory = "library/texture/";
		if (!OS::Directory::exist(directory))
			OS::Directory::create(directory);
		String libPath = directory + name + ".tex";

		// Convert and save
		TextureStorage storage;
		storage.type = gfx::TextureType::TextureCubeMap;
		storage.flags = flags;
		storage.format = gfx::TextureFormat::RGBA8;
		storage.images.push_back(Image::load(px));
		storage.images.push_back(Image::load(py));
		storage.images.push_back(Image::load(pz));
		storage.images.push_back(Image::load(nx));
		storage.images.push_back(Image::load(ny));
		storage.images.push_back(Image::load(nz));
		if (has(flags, gfx::TextureFlag::GenerateMips))
			storage.levels = gfx::Sampler::mipLevelCount(storage.images[0].width(), storage.images[0].height());

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		if (resource->load<Texture>(name, libPath).resource == nullptr)
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
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	String libPath = "library/audio/" + name + ".audio";
	// Convert and save (only copy for now)
	if (!OS::File::create(libPath))
		Logger::error("Failed to create file");
	if (!OS::File::copy(path, libPath))
		Logger::error("Failed to copy file");
	// Load
	resource->load<AudioStream>(name, libPath);
	return true;
}

bool Importer::importFont(const aka::String& name, const aka::Path& path)
{
	Application* app = Application::app();
	ResourceManager* resource = app->resource();
	if (!resource->has<Font>(name))
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
		if (resource->load<Font>(name, libPath).resource == nullptr)
			return false;
	}
	else
	{
		Logger::warn("Font already imported : ", name);
	}
	return true;
}

};