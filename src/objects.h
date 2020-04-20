#ifndef OBJECTS_H
#define OBJECTS_H

#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <array>

#define M_PI 3.14159265358979323846264338327950288

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

class Mesh;

struct BoundingBox {
	float minX, maxX, minY, maxY, minZ, maxZ;
	float intersect(point3 e, point3 d, bool exit = false);
};

struct Material {
	colour3 ambient = colour3(0, 0, 0);
	colour3 diffuse = colour3(0, 0, 0);
	colour3 specular = colour3(0, 0, 0);
	float shininess = 0;
	colour3 reflective = colour3(0, 0, 0);
	colour3 transmissive = colour3(0, 0, 0);
	float refraction = 0;
};

class Light {
public:
	std::string type;
	colour3 colour;
	virtual void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) = 0;
};

class Ambient : public Light {
public:
	Ambient(colour3 colour);
	void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour);
};

class Directional : public Light {
public:
	point3 direction;
	Directional(colour3 colour, point3 direction);
	void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour);
};

class Point : public Light {
public:
	point3 position;
	Point(colour3 colour, point3 position);
	void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour);
};

class Spot : public Light {
public:
	point3 position;
	point3 direction;
	float cutoff;
	Spot(point3 colour, point3 position, point3 direction, float cutoff);
	void lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour);
};

class Object {
public:
	std::string type;
	Material material;
	BoundingBox boundingBox;
	point3 cachedHitpoint;
	virtual float rayhit(point3 e, point3 d, bool exit = false) = 0;
	virtual void getNormal(point3 &n) = 0;
	virtual void getCentroid(point3& c) = 0;
	virtual void lightPoint(point3 e, point3 d, std::vector<Light*> Lights, colour3& colour, int reflectionCount, bool pick);
	virtual bool transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick);
};

class Sphere : public Object {
public:
	point3 center;
	float radius;
	Sphere(point3 center, float radius, Material material);
	float rayhit(point3 e, point3 d, bool exit);
	virtual void getNormal(point3 &n);
	void getCentroid(point3& c);
};

class Plane : public Object {
public:
	point3 point;
	point3 normal;
	Plane(point3 point, point3 normal, Material material);
	float rayhit(point3 e, point3 d, bool exit = false);
	void getNormal(point3 &n);
	void getCentroid(point3& c);
	bool transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick);
};

class Triangle : public Object {
public:
	std::array<point3, 3> points;
	point3 normal;
	Mesh* mesh;
	Triangle(Mesh* mesh, point3 p0, point3 p1, point3 p2, Material material);
	float rayhit(point3 e, point3 d, bool exit);
	void getNormal(point3& n);
	void getCentroid(point3& c);
	bool transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick);
};

class Mesh : public Object {
private:
	point3 cachedHitNormal;
public:
	std::vector<Triangle*> triangles;
	Mesh(Material material);
	float rayhit(point3 e, point3 d, bool exit);
	void getNormal(point3 &n);
	void getCentroid(point3& c);
	void setBox();
};

class Box : public Object {
public:
	Box(BoundingBox box, Material material);
	float rayhit(point3 e, point3 d, bool exit);
	void getNormal(point3& n);
	void getCentroid(point3& c);
};

#endif