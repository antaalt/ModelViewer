#pragma once

#include "Model.h"

namespace app {

struct IWorldImporter
{
	virtual aka::World import(const Path& path) = 0;
	virtual aka::World import(const byte_t* bytes, size_t count) = 0;
};
// AssimpImporter, TinyOBJImporter...


struct Importer {
	// Import a scene using assimp and convert it to a scene.json and add assets to resource manager
	static bool importScene(const Path& path, aka::World& world);
	// Import a mesh and add it to resource manager
	static bool importMesh(const aka::String& name, const aka::Path& path);
	// Import a texture and add it to resource manager
	static bool importTexture2D(const aka::String& name, const aka::Path& path, gfx::TextureFlag flags);
	// Import an HDR texture and add it to resource manager
	static bool importTexture2DHDR(const aka::String& name, const aka::Path& path, gfx::TextureFlag flags);
	// Import a cubemap and add it to resource manager
	static bool importTextureCubemap(const aka::String& name, const aka::Path& px, const aka::Path& py, const aka::Path& pz, const aka::Path& nx, const aka::Path& ny, const aka::Path& nz, gfx::TextureFlag flags);
	// Import an audio and add it to resource manager
	static bool importAudio(const aka::String& name, const aka::Path& path);
	// Import a font and add it to resource manager
	static bool importFont(const aka::String& name, const aka::Path& path);
};

};