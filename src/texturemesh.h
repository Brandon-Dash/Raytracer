#ifndef TEXTUREMESH_H
#define TEXTUREMESH_H

#include "objects.h"
#include "EasyBMP/EasyBMP.h"
#include <array>
#include <string>

typedef glm::vec2 uvCoord;

class TextureMesh : public Mesh {
public:
	BMP texture;
	TextureMesh(Material material, std::string texturefile);
	void getTexValue(float u, float v, colour3& colour);
};

class TextureTriangle : public Triangle {
public:
	std::array<uvCoord, 3> uvCoords;
	TextureTriangle(Mesh* mesh, point3 p0, point3 p1, point3 p2, uvCoord uv0, uvCoord uv1, uvCoord uv2, Material material);
	void lightPoint(point3 e, point3 d, std::vector<Light*> Lights, colour3& colour, int reflectionCount, bool pick);
};

#endif