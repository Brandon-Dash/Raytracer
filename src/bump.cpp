#include "bump.h"

BumpSphere::BumpSphere(point3 center, float radius, Material material, std::string bumpmapfile, float bumpDepth) : Sphere::Sphere(center, radius, material) {
	this->bumpmap.ReadFromFile(bumpmapfile.c_str());
	this->bumpDepth = bumpDepth;
}

void BumpSphere::getNormal(point3& n) {
	//get actual surface normal
	point3 trueNormal;
	Sphere::getNormal(trueNormal);

	// determine UV coords of the hitpoint
	float u = 0.5 - atan2(-trueNormal.z, -trueNormal.x) / (2 * M_PI);
	float v = 0.5 - asin(trueNormal.y) / M_PI;

	// read bumpmap value at UV point
	int height = bumpmap.TellHeight();
	int width = bumpmap.TellWidth();

	float value = float(bumpmap(int(u * width), int(v * height))->Red) / 255;
	float value_u = float(bumpmap(int(u * width + 1) % width, int(v * height))->Red) / 255;
	float value_v = float(bumpmap(int(u * width), int(v * height + 1) % height)->Red) / 255;

	float gradient_u = value_u - value;
	float gradient_v = value_v - value;

	// get surface tangents that correspond to u and v directions
	point3 tangent_u = glm::normalize(glm::cross(point3(0, 1, 0), trueNormal));
	point3 tangent_v = glm::normalize(glm::cross(trueNormal, tangent_u));

	// modify the surface normal based on the bumpmap gradient
	n = glm::normalize(trueNormal + gradient_u * bumpDepth * tangent_u + gradient_v * bumpDepth * tangent_v);
}