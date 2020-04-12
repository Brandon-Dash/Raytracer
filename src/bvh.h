#ifndef BVH_H
#define BVH_H

#include "raytracer.h"

class BVH {
public:
	BVH_node* root;
};

class BVH_node {
	BVH_node* left;
	BVH_node* right;

};

#endif