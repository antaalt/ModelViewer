#include "Model.h"

// tiny_gltf include windows.h...
#if defined(AKA_PLATFORM_WINDOWS)
#undef min
#undef max
#endif

namespace viewer {


void ArcballCamera::set(const aabbox<>& bbox)
{
	float dist = bbox.extent().norm();
	position = bbox.max * 1.2f;
	target = bbox.center();
	up = norm3f(0,1,0);
	transform = aka::mat4f::lookAt(position, target, up);
	speed = dist;
}

};