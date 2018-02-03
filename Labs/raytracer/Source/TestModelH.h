#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>

// Used to describe a triangular surface:
class Triangle
{
public:
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec4 normal;
	glm::vec3 color;
	float shininess;
	float Ks;
	float Kd;
	glm::vec3 e1;
	glm::vec3 e2;

	Triangle( glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3 color, float shininess, float Ks, float Kd )
		: v0(v0), v1(v1), v2(v2), color(color), shininess(shininess), Ks(Ks), Kd(Kd)
	{
		ComputeNormal();
	}

	void ComputeNormal()
	{
	  e1 = glm::vec3(v1 - v0);
	  e2 = glm::vec3(v2 - v0);
	  glm::vec3 normal3 = glm::normalize(glm::cross(e2, e1));
	  normal = glm::vec4(normal3, 1.0f);
	}
};

class Sphere
{
public:
	glm::vec4 c;
	float radius;
	glm::vec3 color;
	float shininess;
	float Ks;
	float Kd;

	Sphere( glm::vec4 c, float radius, glm::vec3 color, float shininess, float Ks, float Kd)
		: c(c), radius(radius), color(color), shininess(shininess), Ks(Ks), Kd(Kd) {
			
		}
};

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel(std::vector<Triangle>& triangles, std::vector<Sphere>& spheres)
{
	using glm::vec3;
	using glm::vec4;

	// Defines colors:
	vec3 red(    0.75f, 0.15f, 0.15f );
	vec3 yellow( 0.75f, 0.75f, 0.15f );
	vec3 green(  0.15f, 0.75f, 0.15f );
	vec3 cyan(   0.15f, 0.75f, 0.75f );
	vec3 blue(   0.15f, 0.15f, 0.75f );
	vec3 purple( 0.75f, 0.15f, 0.75f );
	vec3 white(  0.75f, 0.75f, 0.75f );

	triangles.clear();
	triangles.reserve( 5*2 );
	
	spheres.clear();
	spheres.reserve(2);
	spheres.push_back(Sphere(glm::vec4(-0.45, 0.6, 0.4, 1), 0.4f, white, 2, 0.04, 0.96));
	spheres.push_back(Sphere(glm::vec4(0.6, 0.6, -0.4, 1), 0.3f, white, 2, 0.03, 0.97));

	// ---------------------------------------------------------------------------
	// Room

	float L = 555;			// Length of Cornell Box side.

	vec4 A(L,0,0,1);
	vec4 B(0,0,0,1);
	vec4 C(L,0,L,1);
	vec4 D(0,0,L,1);

	vec4 E(L,L,0,1);
	vec4 F(0,L,0,1);
	vec4 G(L,L,L,1);
	vec4 H(0,L,L,1);

	// Floor:
	triangles.push_back( Triangle( C, B, A, white, 100, 0.25, 0.75 ) );
	triangles.push_back( Triangle( C, D, B, white, 100, 0.25, 0.75 ) );

	// Left wall
	triangles.push_back( Triangle( A, E, C, red, 2, 0.08, 0.92 ) );
	triangles.push_back( Triangle( C, E, G, red, 2, 0.08, 0.92 ) );

	// Right wall
	triangles.push_back( Triangle( F, B, D, green, 2, 0.08, 0.92 ) );
	triangles.push_back( Triangle( H, F, D, green, 2, 0.08, 0.92 ) );

	// Ceiling
	triangles.push_back( Triangle( E, F, G, white, 100, 0.25, 0.75 ) );
	triangles.push_back( Triangle( F, H, G, white, 100, 0.25, 0.75 ) );

	// Back wall
	triangles.push_back( Triangle( G, D, C, white, 100, 0.25, 0.75 ) );
	triangles.push_back( Triangle( G, H, D, white, 100, 0.25, 0.75 ) );

	// ---------------------------------------------------------------------------
	// Short block

	A = vec4(290,0,114,1);
	B = vec4(130,0, 65,1);
	C = vec4(240,0,272,1);
	D = vec4( 82,0,225,1);
	       
	E = vec4(290,165,114,1);
	F = vec4(130,165, 65,1);
	G = vec4(240,165,272,1);
	H = vec4( 82,165,225,1);

	// // Front
	// triangles.push_back( Triangle(E,B,A,red) );
	// triangles.push_back( Triangle(E,F,B,red) );

	// // Front
	// triangles.push_back( Triangle(F,D,B,red) );
	// triangles.push_back( Triangle(F,H,D,red) );

	// // BACK
	// triangles.push_back( Triangle(H,C,D,red) );
	// triangles.push_back( Triangle(H,G,C,red) );

	// // LEFT
	// triangles.push_back( Triangle(G,E,C,red) );
	// triangles.push_back( Triangle(E,A,C,red) );

	// // TOP
	// triangles.push_back( Triangle(G,F,E,red) );
	// triangles.push_back( Triangle(G,H,F,red) );

	// ---------------------------------------------------------------------------
	// Tall block

	A = vec4(423,0,247,1);
	B = vec4(265,0,296,1);
	C = vec4(472,0,406,1);
	D = vec4(314,0,456,1);
	       
	E = vec4(423,330,247,1);
	F = vec4(265,330,296,1);
	G = vec4(472,330,406,1);
	H = vec4(314,330,456,1);

	// // Front
	// triangles.push_back( Triangle(E,B,A,blue) );
	// triangles.push_back( Triangle(E,F,B,blue) );

	// // Front
	// triangles.push_back( Triangle(F,D,B,blue) );
	// triangles.push_back( Triangle(F,H,D,blue) );

	// // BACK
	// triangles.push_back( Triangle(H,C,D,blue) );
	// triangles.push_back( Triangle(H,G,C,blue) );

	// // LEFT
	// triangles.push_back( Triangle(G,E,C,blue) );
	// triangles.push_back( Triangle(E,A,C,blue) );

	// // TOP
	// triangles.push_back( Triangle(G,F,E,blue) );
	// triangles.push_back( Triangle(G,H,F,blue) );


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for( size_t i=0; i<triangles.size(); ++i )
	{
		triangles[i].v0 *= 2/L;
		triangles[i].v1 *= 2/L;
		triangles[i].v2 *= 2/L;

		triangles[i].v0 -= vec4(1,1,1,1);
		triangles[i].v1 -= vec4(1,1,1,1);
		triangles[i].v2 -= vec4(1,1,1,1);

		triangles[i].v0.x *= -1;
		triangles[i].v1.x *= -1;
		triangles[i].v2.x *= -1;

		triangles[i].v0.y *= -1;
		triangles[i].v1.y *= -1;
		triangles[i].v2.y *= -1;

		triangles[i].v0.w = 1.0;
		triangles[i].v1.w = 1.0;
		triangles[i].v2.w = 1.0;
		
		triangles[i].ComputeNormal();
	}
}

#endif
