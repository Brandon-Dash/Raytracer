#include "csg.h"

csgObject::csgObject(Material material) {
	this->material = material;
	type = "csgobject";
}

float csgObject::rayhit(point3 e, point3 d, bool exit) {
	root->setIntervals(e, d);

	float t = 0;

	for (int i = 0; i < root->intervalList.size() && t == 0; i++) {
		if (!exit && root->intervalList[i][0].t > 0) {
			t = root->intervalList[i][0].t;
			cachedHitNormal = root->intervalList[i][0].normal;
		}
		else if (exit && root->intervalList[i][1].t > 0) {
			t = root->intervalList[i][1].t;
			cachedHitNormal = root->intervalList[i][1].normal;
		}
	}

	cachedHitpoint = e + t * d;
	return t;
}

void csgObject::getNormal(point3& n) {
	n = cachedHitNormal;
}

void csgObject::getCentroid(point3& c) {
	c.x = (boundingBox.minX + boundingBox.maxX) / 2;
	c.y = (boundingBox.minY + boundingBox.maxY) / 2;
	c.z = (boundingBox.minZ + boundingBox.maxZ) / 2;
}

void csgObject::setBox() {
	root->getBox(boundingBox);
}

csg_node::csg_node(Operation op) {
	this->op = op;
	object = NULL;
	first = NULL;
	second = NULL;
}

csg_node::csg_node(Object* object) {
	this->object = object;
	first = NULL;
	second = NULL;
	op = NoOp;
}

void csg_node::getBox(BoundingBox& box) {
	if (op == NoOp)
		box = object->boundingBox;

	else if (op == Difference)
		first->getBox(box);

	else {
		BoundingBox box1, box2;
		first->getBox(box1);
		second->getBox(box2);

		if (op == Union) {
			box.minX = std::min(box1.minX, box2.minX);
			box.minY = std::min(box1.minY, box2.minY);
			box.minZ = std::min(box1.minZ, box2.minZ);
			box.maxX = std::max(box1.maxX, box2.maxX);
			box.maxY = std::max(box1.maxY, box2.maxY);
			box.maxZ = std::max(box1.maxZ, box2.maxZ);
		}
		if (op == Intersection) {
			box.minX = std::max(box1.minX, box2.minX);
			box.minY = std::max(box1.minY, box2.minY);
			box.minZ = std::max(box1.minZ, box2.minZ);
			box.maxX = std::min(box1.maxX, box2.maxX);
			box.maxY = std::min(box1.maxY, box2.maxY);
			box.maxZ = std::min(box1.maxZ, box2.maxZ);
		}
	}
}

bool compareIntervals(interval i1, interval i2) {
	return i1[0].t < i2[0].t;
}

bool compareIntersections(intersection i1, intersection i2) {
	return i1.t < i2.t;
}

void csg_node::setIntervals(point3 e, point3 d) {
	//first clear interval list
	intervalList.clear();

	if (object != NULL) {
		// leaf node
		intersection near, far;

		near.t = object->rayhit(e, d, false);
		object->getNormal(near.normal);

		far.t = object->rayhit(e, d, true);
		object->getNormal(far.normal);

		if (far.t > 0)
			intervalList.push_back(interval({ near, far }));
	}

	else {
		// set children intervals then combine them based on this node's operation
		first->setIntervals(e, d);
		second->setIntervals(e, d);

		std::vector<interval> list1 = first->intervalList;
		std::vector<interval> list2 = second->intervalList;
		int count1 = 0;
		int count2 = 0;
		int intervals = 0;

		if (op == Union) {
			if (list1[count1][0].t < list2[count2][0].t) {
				intervalList.push_back(list1[count1]);
				count1++;
				intervals++;
			}
			else {
				intervalList.push_back(list2[count2]);
				count2++;
				intervals++;
			}

			while (count1 < list1.size() && count2 < list2.size()) {
				if (list1[count1][0].t < list2[count2][0].t) {
					if (list1[count1][0].t < intervalList[intervals - 1][1].t) {
						intervalList[intervals - 1][1] = list1[count1][1];
					}
					else {
						intervalList.push_back(list1[count1]);
						intervals++;
					}
					count1++;
				}
				else {
					if (list2[count2][0].t < intervalList[intervals - 1][1].t) {
						intervalList[intervals - 1][1] = list2[count2][1];
					}
					else {
						intervalList.push_back(list2[count2]);
						intervals++;
					}
					count2++;
				}
			}
			while (count1 < list1.size()) {
				if (list1[count1][0].t < intervalList[intervals - 1][1].t) {
					intervalList[intervals - 1][1] = list1[count1][1];
				}
				else {
					intervalList.push_back(list1[count1]);
					intervals++;
				}
				count1++;
			}
			while (count2 < list2.size()) {
				if (list2[count2][0].t < intervalList[intervals - 1][1].t) {
					intervalList[intervals - 1][1] = list2[count2][1];
				}
				else {
					intervalList.push_back(list2[count2]);
					intervals++;
				}
				count2++;
			}
		}

		if (op == Intersection) {
			for (count1 = 0; count1 < list1.size(); count1++) {
				for (count2 = 0; count2 < list2.size(); count2++) {
					if (list1[count1][0].t < list2[count2][1].t && list1[count1][1].t > list2[count2][0].t) {
						intervalList.push_back(interval({ std::max(list1[count1][0], list2[count2][0], compareIntersections), std::min(list1[count1][1], list2[count2][1], compareIntersections) }));
					}
				}
			}
			std::sort(intervalList.begin(), intervalList.end(), compareIntervals);
		}

		if (op == Difference) {
			intersection current;

			for (count1 = 0; count1 < list1.size(); count1++) {
				current = list1[count1][0];

				for (count2 = 0; count2 < list2.size() && current.t < list1[count1][1].t; count2++) {
					interval subtract = list2[count2];
					subtract[0].normal = -subtract[0].normal;
					subtract[1].normal = -subtract[1].normal;


					if (list1[count1][0].t < subtract[1].t && list1[count1][1].t > subtract[0].t) {

						if (subtract[0].t < current.t)
							current = subtract[1];

						else {
							intervalList.push_back(interval({ current, subtract[0] }));
							current = subtract[1];
						}

					}
				}
				if (current.t < list1[count1][1].t)
					intervalList.push_back(interval({ current, list1[count1][1] }));
			}
		}
	}
}