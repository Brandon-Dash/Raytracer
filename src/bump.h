#ifndef BUMP_H
#define BUMP_H

#include "objects.h"
#include "EasyBMP/EasyBMP.h"
#include <string>

class BumpSphere : public Sphere {
public:
	BMP bumpmap;
	float bumpDepth;
	BumpSphere(point3 center, float radius, Material material, std::string bumpmapfile, float bumpDepth);
	void getNormal(point3& n);
};

#endif