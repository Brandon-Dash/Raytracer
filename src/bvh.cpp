#include "bvh.h"

BVH::BVH(std::vector<Object*> objects) {
	// create list of all objects to be put into the tree, and separate out the planes
	std::vector<Object*> objectList;
	
	for (int i = 0; i < objects.size(); i++) {
		Object* currentObject = objects[i];

		if (currentObject->type == "plane") {
			// planes have infinite bounding boxes so they can't be put in the tree
			planes.push_back((Plane*)currentObject);
		}
		else if (currentObject->type == "mesh") {
			// each triangle is inserted into the tree individually
			Mesh* mesh = (Mesh*)currentObject;
			for (int t = 0; t < mesh->triangles.size(); t++) {
				objectList.push_back(mesh->triangles[t]);
			}
		}
		else {
			// all other objects are assumed to be eligible for the tree as-is
			objectList.push_back(currentObject);
		}
	}

	// create the root node, then recursively split it to create the tree

	root = new BVH_node(objectList);

	splitNode(root);
}

void BVH::splitNode(BVH_node* node, int depth) {
	// stop splitting when node has two or fewer objects or maximum depth is reached
	if (node->objects.size() <= 2 || depth >= MAX_BVH_DEPTH)
		return;

	// sort objects by position along longest axis

	float lengthX = node->boundingBox.maxX - node->boundingBox.minX;
	float lengthY = node->boundingBox.maxY - node->boundingBox.minY;
	float lengthZ = node->boundingBox.maxZ - node->boundingBox.minZ;

	if (lengthX >= lengthY && lengthX >= lengthZ) {
		node->sortObjects(0);
	}
	else if (lengthY <= lengthZ) {
		node->sortObjects(1);
	}
	else {
		node->sortObjects(2);
	}

	// split objects into two lists

	std::vector<Object*> leftObjects(node->objects.begin(), node->objects.begin() + node->objects.size() / 2);
	std::vector<Object*> rightObjects(node->objects.begin() + node->objects.size() / 2, node->objects.end());

	// create children nodes and recursively split them

	node->left = new BVH_node(leftObjects);
	node->right = new BVH_node(rightObjects);

	splitNode(node->left, depth + 1);
	splitNode(node->right, depth + 1);
}

BVH_node::BVH_node(std::vector<Object*> objects) {
	// go through each object to set total bounding box for this node
	Object* currentObject = objects[0];
	boundingBox = currentObject->boundingBox;
	this->objects.push_back(currentObject);

	for (int i = 1; i < objects.size(); i++) {
		currentObject = objects[i];

		boundingBox.minX = std::min(boundingBox.minX, currentObject->boundingBox.minX);
		boundingBox.maxX = std::max(boundingBox.maxX, currentObject->boundingBox.maxX);
		boundingBox.minY = std::min(boundingBox.minY, currentObject->boundingBox.minY);
		boundingBox.maxY = std::max(boundingBox.maxY, currentObject->boundingBox.maxY);
		boundingBox.minZ = std::min(boundingBox.minZ, currentObject->boundingBox.minZ);
		boundingBox.maxZ = std::max(boundingBox.maxZ, currentObject->boundingBox.maxZ);

		this->objects.push_back(currentObject);
	}

	left = NULL;
	right = NULL;
}

bool compareX(Object* o1, Object* o2) {
	point3 c1, c2;
	o1->getCentroid(c1);
	o2->getCentroid(c2);
	return c1.x < c2.x;
}
bool compareY(Object* o1, Object* o2) {
	point3 c1, c2;
	o1->getCentroid(c1);
	o2->getCentroid(c2);
	return c1.y < c2.y;
}
bool compareZ(Object* o1, Object* o2) {
	point3 c1, c2;
	o1->getCentroid(c1);
	o2->getCentroid(c2);
	return c1.z < c2.z;
}

void BVH_node::sortObjects(int axis) {
	if (axis == 0) {
		std::sort(objects.begin(), objects.end(), compareX);
	}
	else if (axis == 1) {
		std::sort(objects.begin(), objects.end(), compareY);
	}
	else {
		std::sort(objects.begin(), objects.end(), compareZ);
	}
}