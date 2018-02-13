#include <glm/glm.hpp>
#include <vector>

#include "objects.h"

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel(std::vector<Object *> &scene) {
  using glm::vec3;
  using glm::vec4;

  // Defines colors:
  vec3 red(0.75f, 0.15f, 0.15f);
  vec3 yellow(0.75f, 0.75f, 0.15f);
  vec3 green(0.15f, 0.75f, 0.15f);
  vec3 cyan(0.15f, 0.75f, 0.75f);
  vec3 blue(0.15f, 0.15f, 0.75f);
  vec3 purple(0.75f, 0.15f, 0.75f);
  vec3 white(0.75f, 0.75f, 0.75f);

  // ---------------------------------------------------------------------------
  // Sphere 1
  std::vector<Primitive *> sphere1Primitives;
  sphere1Primitives.push_back(new Sphere(vec4(-0.45, 0.6, 0.4, 1), 0.4f,
                                         vec3(0), white, 2, 0.5, 0.04, 0.46));
  scene.push_back(new Object(sphere1Primitives));

  // ---------------------------------------------------------------------------
  // Sphere 2
  std::vector<Primitive *> sphere2Primitives;
  sphere2Primitives.push_back(new Sphere(vec4(0.6, 0.6, -0.4, 1), 0.3f, vec3(0),
                                         white, 2, 0.5, 0.04, 0.46));
  scene.push_back(new Object(sphere2Primitives));

  // ---------------------------------------------------------------------------
  // Room

  float L = 555;  // Length of Cornell Box side.

  vec4 A(L, 0, 0, 1);
  vec4 B(0, 0, 0, 1);
  vec4 C(L, 0, L, 1);
  vec4 D(0, 0, L, 1);

  vec4 E(L, L, 0, 1);
  vec4 F(0, L, 0, 1);
  vec4 G(L, L, L, 1);
  vec4 H(0, L, L, 1);

  // Light
  std::vector<Primitive *> lightPrimitives;
  lightPrimitives.push_back(
      new Triangle(vec4(3 * L / 5, 0.99 * L, 2 * L / 5, 1),
                   vec4(2 * L / 5, 0.99 * L, 2 * L / 5, 1),
                   vec4(3 * L / 5, 0.99 * L, 3 * L / 5, 1), 50.0f * vec3(1),
                   vec3(0), 1, 0.1, 0.1, 0.8));
  lightPrimitives.push_back(
      new Triangle(vec4(2 * L / 5, 0.99 * L, 2 * L / 5, 1),
                   vec4(2 * L / 5, 0.99 * L, 3 * L / 5, 1),
                   vec4(3 * L / 5, 0.99 * L, 3 * L / 5, 1), 50.0f * vec3(1),
                   vec3(0), 1, 0.1, 0.1, 0.8));
  scene.push_back(new Object(lightPrimitives));

  // Floor:
  std::vector<Primitive *> floorPrimitives;
  floorPrimitives.push_back(
      new Triangle(C, B, A, vec3(0), white, 10, 0.5, 0.06, 0.44));
  floorPrimitives.push_back(
      new Triangle(C, D, B, vec3(0), white, 10, 0.5, 0.06, 0.44));
  scene.push_back(new Object(floorPrimitives));

  // Left wall
  std::vector<Primitive *> leftWallPrimitives;
  leftWallPrimitives.push_back(
      new Triangle(A, E, C, vec3(0), red, 2, 0.5, 0.04, 0.46));
  leftWallPrimitives.push_back(
      new Triangle(C, E, G, vec3(0), red, 2, 0.5, 0.04, 0.46));
  scene.push_back(leftWallPrimitives);

  // Right wall
  std::vector<Primitive *> rightWallPrimitives;
  rightWallPrimitives.push_back(
      new Triangle(F, B, D, vec3(0), green, 2, 0.5, 0.04, 0.46));
  rightWallPrimitives.push_back(
      new Triangle(H, F, D, vec3(0), green, 2, 0.5, 0.04, 0.46));
  scene.push_back(rightWallPrimitives);

  // Ceiling
  std::vector<Primitive *> ceilingPrimitives;
  ceilingPrimitives.push_back(
      new Triangle(E, F, G, vec3(0), white, 10, 0.5, 0.46, 0.04));
  ceilingPrimitives.push_back(
      new Triangle(F, H, G, vec3(0), white, 10, 0.5, 0.46, 0.04));
  scene.push_back(ceilingPrimitives);

  // Back wall
  std::vector<Primitive *> backWallPrimitives;
  backWallPrimitives.push_back(
      new Triangle(G, D, C, vec3(0), white, 10, 0.5, 0.06, 0.44));
  backWallPrimitives.push_back(
      new Triangle(G, H, D, vec3(0), white, 10, 0.5, 0.06, 0.44));
  scene.push_back(backWallPrimitives);

  // ---------------------------------------------------------------------------
  // Short block

  A = vec4(290, 0, 114, 1);
  B = vec4(130, 0, 65, 1);
  C = vec4(240, 0, 272, 1);
  D = vec4(82, 0, 225, 1);

  E = vec4(290, 165, 114, 1);
  F = vec4(130, 165, 65, 1);
  G = vec4(240, 165, 272, 1);
  H = vec4(82, 165, 225, 1);

  // // Front
  // primitives.push_back( Triangle(E,B,A,red) );
  // primitives.push_back( Triangle(E,F,B,red) );

  // // Front
  // primitives.push_back( Triangle(F,D,B,red) );
  // primitives.push_back( Triangle(F,H,D,red) );

  // // BACK
  // primitives.push_back( Triangle(H,C,D,red) );
  // primitives.push_back( Triangle(H,G,C,red) );

  // // LEFT
  // primitives.push_back( Triangle(G,E,C,red) );
  // primitives.push_back( Triangle(E,A,C,red) );

  // // TOP
  // primitives.push_back( Triangle(G,F,E,red) );
  // primitives.push_back( Triangle(G,H,F,red) );

  // ---------------------------------------------------------------------------
  // Tall block

  A = vec4(423, 0, 247, 1);
  B = vec4(265, 0, 296, 1);
  C = vec4(472, 0, 406, 1);
  D = vec4(314, 0, 456, 1);

  E = vec4(423, 330, 247, 1);
  F = vec4(265, 330, 296, 1);
  G = vec4(472, 330, 406, 1);
  H = vec4(314, 330, 456, 1);

  // // Front
  // primitives.push_back( Triangle(E,B,A,blue) );
  // primitives.push_back( Triangle(E,F,B,blue) );

  // // Front
  // primitives.push_back( Triangle(F,D,B,blue) );
  // primitives.push_back( Triangle(F,H,D,blue) );

  // // BACK
  // primitives.push_back( Triangle(H,C,D,blue) );
  // primitives.push_back( Triangle(H,G,C,blue) );

  // // LEFT
  // primitives.push_back( Triangle(G,E,C,blue) );
  // primitives.push_back( Triangle(E,A,C,blue) );

  // // TOP
  // primitives.push_back( Triangle(G,F,E,blue) );
  // primitives.push_back( Triangle(G,H,F,blue) );

  // ----------------------------------------------
  // Scale to the volume [-1,1]^3

  for (size_t i = 0; i < scene.size(); ++i) {
    for (size_t j = 0; j < scene[i].primitives.size(); j++) {
      Triangle *tri;
      if ((tri = dynamic_cast<Triangle *>(scene[i].primitives[j]))) {
        tri->v0 *= 2 / L;
        tri->v1 *= 2 / L;
        tri->v2 *= 2 / L;

        tri->v0 -= vec4(1, 1, 1, 1);
        tri->v1 -= vec4(1, 1, 1, 1);
        tri->v2 -= vec4(1, 1, 1, 1);

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
}
