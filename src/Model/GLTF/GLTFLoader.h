#pragma once

#include "../ModelLoader.h"

namespace viewer {

class GLTFLoader : public ModelLoader
{
public:
	Model::Ptr load(const Path& path) override;
};

};