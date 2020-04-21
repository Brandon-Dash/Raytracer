#ifndef AREALIGHT_H
#define AREALIGHT_H

#include "objects.h"

class AreaLight : public Light {
public:
	point3 position;
	point3 normal;
	point3 planeX;
	point3 planeY;
	int numSamples;
	void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour);
	virtual void randomPoint(point3 &p) = 0;
	virtual void preparePoints() = 0;
	virtual void nextPoint(point3& p) = 0;
};

class RectangularAreaLight : public AreaLight {
public:
	float width;
	float height;
	RectangularAreaLight(colour3 colour, point3 position, point3 normal, float width, float height, point3 orientation, int numSamples);
	void randomPoint(point3& p);
	void preparePoints();
	void nextPoint(point3& p);
private:
	float x;
	float y;
	float step;
};

class CircularAreaLight : public AreaLight {
public:
	float radius;
	CircularAreaLight(colour3 colour, point3 position, point3 normal, float radius, int numSamples);
	void randomPoint(point3& p);
	void preparePoints();
	void nextPoint(point3& p);
};

#endif