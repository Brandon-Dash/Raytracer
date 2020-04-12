#ifndef BVH_H
#define BVH_H

#include "objects.h"

#define MAX_BVH_DEPTH 16

class BVH_node {
public:
	BVH_node* left;
	BVH_node* right;
	BoundingBox boundingBox;
	std::vector<Object*> objects;
	BVH_node(std::vector<Object*> objects);
	void sortObjects(int axis);
};

class BVH {
public:
	BVH_node* root;
	std::vector<Plane*> planes;
	BVH(std::vector<Object*> objects);
private:
	void splitNode(BVH_node* node, int depth = 0);
};

#endif