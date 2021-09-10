#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

#include <filesystem>

namespace viewer {

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
	Entity root = m_world.createEntity("Imported mesh");
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
		// Index buffer
		String indexBufferName = meshName + "-indices";
		Path indexBufferPath = "library/buffer/" + indexBufferName + ".buffer";
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
		Path vertexBufferPath = "library/buffer/" + vertexBufferName + ".buffer";
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
		Path meshPath = "library/mesh/" + meshName + ".mesh";
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
	String name = file::name(path);
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

bool ModelLoader::load(const Path& path, aka::World& world)
{
	return Importer::importScene(path, world);
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
	String directory = path.str().substr(0, path.str().findLast('/'));
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
		String libPath = "library/texture/" + name + ".tex";

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
		ResourceManager::load<Texture>(name, libPath);
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
		String libPath = "library/texture/" + name + ".tex";

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
		ResourceManager::load<Texture>(name, libPath);
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
		String libPath = "library/texture/" + name + ".tex";

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
		ResourceManager::load<Texture>(name, libPath);
	}
	else
	{
		Logger::warn("Texture already imported : ", name);
	}
	return true;
}

#if defined(AKA_USE_OPENGL)
const char* vertShader = "#version 330 core\n"
"layout(location = 0) in vec3 a_position;\n"
"layout(std140) uniform CameraUniformBuffer { mat4 projection; mat4 view; };\n"
"out vec3 v_position;\n"
"void main()\n"
"{\n"
"	v_position = a_position;\n"
"	gl_Position = projection * view * vec4(a_position, 1.0);\n"
"}\n";

const char* fragShader = "#version 330 core\n"
"in vec3 v_position;\n"
"uniform sampler2D u_environmentMap;\n"
"out vec4 o_color;\n"
"const vec2 invAtan = vec2(0.1591, 0.3183);\n"
"vec2 SampleSphericalMap(vec3 v)\n"
"{\n"
"	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));\n"
"	uv *= invAtan;\n"
"	uv += 0.5;\n"
"	return uv;\n"
"}\n"
"void main()\n"
"{\n"
"	vec2 uv = SampleSphericalMap(normalize(v_position));\n"
"	vec3 color = texture(u_environmentMap, uv).rgb;\n"
"	o_color = vec4(color, 1.0);\n"
"}\n";
#elif defined(AKA_USE_D3D11)
const char* shader = ""
"cbuffer CameraUniformBuffer : register(b0) { float4x4 projection; float4x4 view; };\n"
"struct vs_out { float4 position : SV_POSITION; float3 localPosition : POS; };\n"
"Texture2D    u_environmentMap : register(t0);\n"
"SamplerState u_environmentMapSampler : register(s0);\n"
"static const float2 invAtan = float2(0.1591, 0.3183);\n"
"float2 SampleSphericalMap(float3 v)\n"
"{\n"
"	float2 uv = float2(atan2(v.z, v.x), asin(v.y));\n"
"	uv *= invAtan;\n"
"	uv += 0.5;\n"
"	return uv;\n"
"}\n"
"vs_out vs_main(float3 position : POS)\n"
"{\n"
"	vs_out output;\n"
"	output.localPosition = position;\n"
"	output.position = mul(projection, mul(view, float4(position.x, position.y, position.z, 1.0)));\n"
"	return output;\n"
"}\n"
"float4 ps_main(vs_out input) : SV_TARGET\n"
"{\n"
"	float2 uv = SampleSphericalMap(normalize(input.localPosition));\n"
"	float3 color = u_environmentMap.Sample(u_environmentMapSampler, uv).rgb;\n"
"	return float4(color.x, color.y, color.z, 1.0);\n"
"}\n";
const char* vertShader = shader;
const char* fragShader = shader;
#endif

bool Importer::importTextureEnvmap(const aka::String& name, const aka::Path& path, TextureFlag flags)
{
	if (!ResourceManager::has<Texture>(name))
	{
		String libPath = "library/texture/" + name + ".tex";
		// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
		// TODO cubemap resolution depends on envmap resolution ?
		uint32_t cubemapWidth = 1024;
		uint32_t cubemapHeight = 1024;
		// Load envmap
		Image imageHDR = Image::loadHDR(path);
		if (imageHDR.size() == 0 || imageHDR.format() != ImageFormat::Float)
		{
			Logger::error("Image failed to load.");
			return false;
		}
		// Convert it to cubemap
		Texture2D::Ptr envmap = Texture2D::create(imageHDR.width(), imageHDR.height(), TextureFormat::RGBA32F, TextureFlag::ShaderResource, imageHDR.data());
		TextureCubeMap::Ptr cubemap = TextureCubeMap::create(cubemapWidth, cubemapHeight, TextureFormat::RGBA32F, TextureFlag::RenderTarget);
		// Create framebuffer
		Attachment attachment = Attachment{ AttachmentType::Color0, cubemap, AttachmentFlag::None, 0, 0 };
		Framebuffer::Ptr framebuffer = Framebuffer::create(&attachment, 1);

		// Meshes
		float skyboxVertices[] = {
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f,  1.0f
		};
		Mesh::Ptr cube = Mesh::create();
		VertexAccessor skyboxVertexInfo = {
			VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
			VertexBufferView{
				Buffer::create(BufferType::Vertex, sizeof(skyboxVertices), BufferUsage::Immutable, BufferCPUAccess::None, skyboxVertices),
				0, // offset
				sizeof(skyboxVertices), // size
				sizeof(float) * 3 // stride
			},
			0, // offset
			sizeof(skyboxVertices) / (sizeof(float) * 3) // count
		};
		cube->upload(&skyboxVertexInfo, 1);

		// Transforms
		mat4f captureProjection = mat4f::perspective(anglef::degree(90.0f), 1.f, 0.1f, 10.f);
		mat4f captureViews[6] = {
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(1.0f,  0.0f,  0.0f), norm3f(0.0f, -1.0f,  0.0f))),
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(-1.0f,  0.0f,  0.0f), norm3f(0.0f, -1.0f,  0.0f))),
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(0.0f,  1.0f,  0.0f), norm3f(0.0f,  0.0f,  1.0f))),
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(0.0f, -1.0f,  0.0f), norm3f(0.0f,  0.0f, -1.0f))),
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(0.0f,  0.0f,  1.0f), norm3f(0.0f, -1.0f,  0.0f))),
			mat4f::inverse(mat4f::lookAt(point3f(0.0f, 0.0f, 0.0f), point3f(0.0f,  0.0f, -1.0f), norm3f(0.0f, -1.0f,  0.0f)))
		};
		// Shader
		VertexAttribute attribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 };
		ShaderHandle vert = Shader::compile(vertShader, ShaderType::Vertex);
		ShaderHandle frag = Shader::compile(fragShader, ShaderType::Fragment);
		Shader::Ptr shader = Shader::createVertexProgram(vert, frag, &attribute, 1);
		ShaderMaterial::Ptr material = ShaderMaterial::create(shader);
		Shader::destroy(vert);
		Shader::destroy(frag);

		RenderPass pass{};
		pass.framebuffer = framebuffer;
		pass.clear = Clear{ ClearMask::Color | ClearMask::Depth, color4f(0.f), 1.f, 0 };
		pass.blend = Blending::none;
		pass.cull = Culling::none;
		pass.depth = Depth::none;
		pass.stencil = Stencil::none;
		pass.scissor = Rect{ 0 };
		pass.viewport = Rect{ 0, 0, cubemapWidth, cubemapHeight };
		pass.material = material;
		pass.submesh.mesh = cube;
		pass.submesh.type = PrimitiveType::Triangles;
		pass.submesh.count = cube->getVertexCount(0);
		pass.submesh.offset = 0;

		struct CameraUniformBuffer {
			mat4f projection;
			mat4f view;
		} camera;
		camera.projection = captureProjection;

		Buffer::Ptr cameraUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(CameraUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);

		pass.material->set("u_environmentMap", TextureSampler::bilinear);
		pass.material->set("u_environmentMap", envmap);
		pass.material->set("CameraUniformBuffer", cameraUniformBuffer);

		// Convert and save
		TextureStorage storage;
		storage.type = TextureType::TextureCubeMap;
		storage.flags = flags;
		storage.format = TextureFormat::RGBA32F;
		Image img(cubemapWidth, cubemapHeight, 4, ImageFormat::Float);
		for (int i = 0; i < 6; ++i)
		{
			camera.view = captureViews[i];
			cameraUniformBuffer->upload(&camera);
			framebuffer->set(AttachmentType::Color0, cubemap, AttachmentFlag::None, i);
			pass.execute();
			cubemap->download(img.data(), i);
			// ---
			img.encodeHDR(Path("./envmap" + std::to_string(i) + ".hdr"));
			// ---
			storage.images.emplace_back(img);
		}

		// blabla
		if (!storage.save(libPath))
			return false;
		// Load
		ResourceManager::load<Texture>(name, libPath);
	}
	else
	{
		Logger::warn("Environment map already imported : ", name);
	}
	return true;
}

bool Importer::importAudio(const aka::String& name, const aka::Path& path)
{
	String libPath = "library/audio/" + name + ".audio";
	// Convert and save (only copy for now)
	if (!file::create(libPath))
		Logger::error("Failed to create file");
	std::filesystem::copy(path.cstr(), libPath.cstr());
	// Load
	ResourceManager::load<AudioStream>(name, libPath);
	return true;
}

bool Importer::importFont(const aka::String& name, const aka::Path& path)
{
	String libPath = "library/font/" + name + ".font";
	// Convert and save (only copy for now)
	if (!file::create(libPath))
		Logger::error("Failed to create file");
	std::filesystem::copy(path.cstr(), libPath.cstr());
	// Load
	ResourceManager::load<Font>(name, libPath);
	return true;
}

};