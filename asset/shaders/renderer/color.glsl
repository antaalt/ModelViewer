#ifndef COLOR_H
#define COLOR_H

// Linear space to sRGB space conversion
float linear2sRGBChannel(float linear)
{
	return pow(linear, 1.f / 2.2f);
}
vec3 linear2sRGB(vec3 linear)
{
	return vec3(
		linear2sRGBChannel(linear.x),
		linear2sRGBChannel(linear.y),
		linear2sRGBChannel(linear.z)
	);
}
vec4 linear2sRGB(vec4 linear)
{
	return vec4(
		linear2sRGBChannel(linear.x),
		linear2sRGBChannel(linear.y),
		linear2sRGBChannel(linear.z),
		linear.w
	);
}
// sRGB space to linear space conversion
float sRGB2LinearChannel(float sRGB)
{
	return pow(sRGB, 2.2f);
}
vec3 sRGB2Linear(vec3 sRGB)
{
	return vec3(
		sRGB2LinearChannel(sRGB.x),
		sRGB2LinearChannel(sRGB.y),
		sRGB2LinearChannel(sRGB.z)
	);
}
vec4 sRGB2Linear(vec4 sRGB)
{
	return vec4(
		sRGB2LinearChannel(sRGB.x),
		sRGB2LinearChannel(sRGB.y),
		sRGB2LinearChannel(sRGB.z),
		sRGB.w
	);
}
#endif
