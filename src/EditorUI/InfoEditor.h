#pragma once

#include "EditorWindow.h"

namespace app {

class InfoEditor : public EditorWindow
{
public:
	void onRender(aka::World& world, aka::Frame* frame) override;
};

};