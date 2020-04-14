#include "texturemesh.h"

TextureMesh::TextureMesh(Material material, std::string texturefile) : Mesh::Mesh(material) {
	this->texture.ReadFromFile(texturefile.c_str());
}

void TextureMesh::getTexValue(float u, float v, colour3& colour) {
	// get the colour value at coordinates (u,v) in the texture map
	int height = texture.TellHeight();
	int width = texture.TellWidth();

	RGBApixel* pixel = texture(int(u * width), int(v * height));
	colour.r = float(pixel->Red) / 255;
	colour.g = float(pixel->Green) / 255;
	colour.b = float(pixel->Blue) / 255;
}

TextureTriangle::TextureTriangle(Mesh* mesh, point3 p0, point3 p1, point3 p2, uvCoord uv0, uvCoord uv1, uvCoord uv2, Material material) :
	Triangle::Triangle(mesh, p0, p1, p2, material) {
	uvCoords[0] = uv0;
	uvCoords[1] = uv1;
	uvCoords[2] = uv2;
}

void TextureTriangle::lightPoint(point3 e, point3 d, std::vector<Light*> Lights, colour3& colour, int reflectionCount, bool pick) {
	// Set diffuse and ambient material properties to texture value, then call normal light function

	point3 p = cachedHitpoint;

	// calculate barycentric coordinates
	point3 v0 = points[0] - p;
	point3 v1 = points[1] - p;
	point3 v2 = points[2] - p;

	float area = glm::length(glm::cross(points[0] - points[1], points[0] - points[2]));
	float a0 = glm::length(glm::cross(v1, v2)) / area;
	float a1 = glm::length(glm::cross(v2, v0)) / area;
	float a2 = glm::length(glm::cross(v0, v1)) / area;

	// interpolate u,v coordinates
	uvCoord uv = uvCoords[0] * a0 + uvCoords[1] * a1 + uvCoords[2] * a2;

	// get colour data from mesh
	TextureMesh* mesh = (TextureMesh*)this->mesh;
	colour3 texColour;
	mesh->getTexValue(uv[0], uv[1], texColour);

	// set ambient and diffuse colours, then call superclass light function
	material.ambient = texColour;
	material.diffuse = texColour;
	Triangle::lightPoint(e, d, Lights, colour, reflectionCount, pick);
}

