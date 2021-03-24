#pragma once

#include "Model.h"

namespace viewer {

class ModelLoader
{
public:
	static Model::Ptr load(const Path& path, const point3f& origin = point3f(0.f), float scale = 1.f);
};

};