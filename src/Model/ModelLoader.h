#pragma once

#include "Model.h"

namespace viewer {

class ModelLoader
{
public:
	static bool load(const Path& path, aka::World& world);
};

};