#pragma once

#include "Editor.h"

namespace viewer {

class AssetEditor : public EditorWindow
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;
	void onUpdate(aka::World& world) override;
	void onRender(aka::World& world) override;
private:
	void import(std::function<bool(const aka::Path&)> callback); // TODO add extension filter
private:
	aka::Path m_currentPath;
	aka::Path* m_selectedPath;
	std::vector<aka::Path> m_paths;
	std::function<bool(const aka::Path& path)> m_importCallback;
};

};