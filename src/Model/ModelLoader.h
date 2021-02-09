#pragma once

#include "Model.h"

namespace viewer {

std::string readString(const aka::Path& path);

class ModelLoader
{
public:
	virtual Model::Ptr load(const Path& path) = 0;
};

};