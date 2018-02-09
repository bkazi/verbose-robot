#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

#include <glm/glm.hpp>
#include <vector>
#include "Objects.h"

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel(std::vector<Shape *>& shapes)
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

	shapes.clear();
	shapes.reserve(6*2 + 2);

	shapes.push_back(new Sphere(vec4(-0.45, 0.6, 0.4, 1), 0.4f, vec3(0), white, 2, 0.5, 0.04, 0.46));
	shapes.push_back(new Sphere(vec4(0.6, 0.6, -0.4, 1), 0.3f, vec3(0), white, 2, 0.5, 0.04, 0.46));

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

	//Light
	shapes.push_back(new Triangle(vec4(3*L/5, 0.99*L, 2*L/5, 1), vec4(2*L/5, 0.99*L, 2*L/5, 1), vec4(3*L/5, 0.99*L, 3*L/5, 1), 50.0f * vec3(1), vec3(0), 1, 0.1, 0.1, 0.8));
	shapes.push_back(new Triangle(vec4(2*L/5, 0.99*L, 2*L/5, 1), vec4(2*L/5, 0.99*L, 3*L/5, 1), vec4(3*L/5, 0.99*L, 3*L/5, 1), 50.0f * vec3(1), vec3(0), 1, 0.1, 0.1, 0.8));

	// Floor:
	shapes.push_back(new Triangle(C, B, A, vec3(0), white, 10, 0.5, 0.06, 0.44));
	shapes.push_back(new Triangle(C, D, B, vec3(0), white, 10, 0.5, 0.06, 0.44));

	// Left wall
	shapes.push_back(new Triangle(A, E, C, vec3(0), red, 2, 0.5, 0.04, 0.46));
	shapes.push_back(new Triangle(C, E, G, vec3(0), red, 2, 0.5, 0.04, 0.46));

	// Right wall
	shapes.push_back(new Triangle(F, B, D, vec3(0), green, 2, 0.5, 0.04, 0.46));
	shapes.push_back(new Triangle(H, F, D, vec3(0), green, 2, 0.5, 0.04, 0.46));

	// Ceiling
	shapes.push_back(new Triangle(E, F, G, vec3(0), white, 10, 0.5, 0.46, 0.04));
	shapes.push_back(new Triangle(F, H, G, vec3(0), white, 10, 0.5, 0.46, 0.04));

	// Back wall
	shapes.push_back(new Triangle(G, D, C, vec3(0), white, 10, 0.5, 0.06, 0.44));
	shapes.push_back(new Triangle(G, H, D, vec3(0), white, 10, 0.5, 0.06, 0.44));

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
	// shapes.push_back( Triangle(E,B,A,red) );
	// shapes.push_back( Triangle(E,F,B,red) );

	// // Front
	// shapes.push_back( Triangle(F,D,B,red) );
	// shapes.push_back( Triangle(F,H,D,red) );

	// // BACK
	// shapes.push_back( Triangle(H,C,D,red) );
	// shapes.push_back( Triangle(H,G,C,red) );

	// // LEFT
	// shapes.push_back( Triangle(G,E,C,red) );
	// shapes.push_back( Triangle(E,A,C,red) );

	// // TOP
	// shapes.push_back( Triangle(G,F,E,red) );
	// shapes.push_back( Triangle(G,H,F,red) );

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
	// shapes.push_back( Triangle(E,B,A,blue) );
	// shapes.push_back( Triangle(E,F,B,blue) );

	// // Front
	// shapes.push_back( Triangle(F,D,B,blue) );
	// shapes.push_back( Triangle(F,H,D,blue) );

	// // BACK
	// shapes.push_back( Triangle(H,C,D,blue) );
	// shapes.push_back( Triangle(H,G,C,blue) );

	// // LEFT
	// shapes.push_back( Triangle(G,E,C,blue) );
	// shapes.push_back( Triangle(E,A,C,blue) );

	// // TOP
	// shapes.push_back( Triangle(G,F,E,blue) );
	// shapes.push_back( Triangle(G,H,F,blue) );


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for(size_t i=0; i < shapes.size(); ++i) {

		Triangle *tri;
		if ((tri = dynamic_cast<Triangle *>(shapes[i]))) {
			tri->v0 *= 2/L;
			tri->v1 *= 2/L;
			tri->v2 *= 2/L;

			tri->v0 -= vec4(1,1,1,1);
			tri->v1 -= vec4(1,1,1,1);
			tri->v2 -= vec4(1,1,1,1);

			tri->v0.x *= -1;
			tri->v1.x *= -1;
			tri->v2.x *= -1;

			tri->v0.y *= -1;
			tri->v1.y *= -1;
			tri->v2.y *= -1;

			tri->v0.w = 1.0;
			tri->v1.w = 1.0;
			tri->v2.w = 1.0;
			
			tri->ComputeNormal();
		}
	}
}

#endif
