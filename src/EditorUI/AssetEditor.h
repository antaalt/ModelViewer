#pragma once

#include "EditorWindow.h"
#include "AssetViewerEditor.h"

namespace app {

class AssetEditor : public EditorWindow
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;
	void onUpdate(aka::World& world, aka::Time deltaTime) override;
	void onRender(aka::World& world, aka::Frame* frame) override;
private:
	void import(std::function<bool(const aka::Path&)> callback); // TODO add extension filter
	//void drawResource(const char* type, aka::ResourceAllocator<T>& resources);
private:
	aka::Path m_currentPath;
	aka::Path* m_selectedPath;
	std::vector<aka::Path> m_paths;
	std::function<bool(const aka::Path& path)> m_importCallback;
	TextureViewerEditor m_textureEditor;
	BufferViewerEditor m_bufferEditor;
	MeshViewerEditor m_meshEditor;
	FontViewerEditor m_fontEditor;
	AudioViewerEditor m_audioEditor;
	std::vector<EditorWindow*> m_viewers;
};

};