#include "Model.h"

// tiny_gltf include windows.h...
#if defined(AKA_PLATFORM_WINDOWS)
#undef min
#undef max
#endif

namespace viewer {

BoundingBox::BoundingBox() :
	min(std::numeric_limits<float>::max()),
	max(-std::numeric_limits<float>::max())
{
}

void BoundingBox::include(const aka::point3f& vec) {
	if (vec.x > max.x) max.x = vec.x;
	if (vec.y > max.y) max.y = vec.y;
	if (vec.z > max.z) max.z = vec.z;
	if (vec.x < min.x) min.x = vec.x;
	if (vec.y < min.y) min.y = vec.y;
	if (vec.z < min.z) min.z = vec.z;
}
void BoundingBox::include(const BoundingBox& bbox) {
	include(bbox.min);
	include(bbox.max);
}
bool BoundingBox::contains(const aka::point3f& point) const
{
	return (point.x > min.x && point.y > min.y && point.z > min.z) && (point.x < max.x&& point.y < max.y&& point.z < max.z);
}
bool BoundingBox::contains(const BoundingBox& aabbox) const
{
	return contains(aabbox.min) && contains(aabbox.max);
}
aka::point3f BoundingBox::center() const
{
	return (min + max) * 0.5f;
}
aka::vec3f BoundingBox::extent() const
{
	return aka::vec3f(max - min);
}
bool BoundingBox::valid() const
{
	return (min.x < max.x&& min.y < max.y&& min.z < max.z);
}

void ArcballCamera::set(const BoundingBox& bbox)
{
	float dist = bbox.extent().norm();
	position = bbox.max * 1.2f;
	target = bbox.center();
	up = norm3f(0,1,0);
	transform = aka::mat4f::lookAt(position, target, up);
	speed = dist;
}

};