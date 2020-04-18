#ifndef CSG_H
#define CSG_H

#include "objects.h"

enum Operation {Union, Intersection, Difference, NoOp};

struct intersection {
	float t;
	point3 normal;
};

typedef std::array<intersection, 2> interval;

class csg_node {
public:
	csg_node* first;
	csg_node* second;
	Operation op;
	Object* object;
	std::vector<interval> intervalList;
	csg_node(Operation op);
	csg_node(Object* object);
	void getBox(BoundingBox& box);
	void setIntervals(point3 e, point3 d);
};

class csgObject : public Object {
public:
	csg_node* root;
	point3 cachedHitNormal;
	csgObject(Material material);
	float rayhit(point3 e, point3 d, bool exit);
	void getNormal(point3& n);
	void getCentroid(point3& c);
	void setBox();
};

#endif