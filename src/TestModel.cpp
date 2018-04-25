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

  // Define materials

  Material sphereMaterial;
  sphereMaterial.color = cyan;
  sphereMaterial.ambient = vec3(0.5);
  sphereMaterial.diffuse = vec3(0.46);
  sphereMaterial.specular = vec3(0.04);
  sphereMaterial.emission = vec3(0);
  sphereMaterial.shininess = 2.f;

  Material lightMaterial;
  lightMaterial.color = vec3(0);
  lightMaterial.ambient = vec3(0.1);
  lightMaterial.diffuse = vec3(0.8);
  lightMaterial.specular = vec3(0.1);
  lightMaterial.emission = 50.f * vec3(1);
  lightMaterial.shininess = 1.f;

//   emit
//   color
//   shininess
//   a
//   s
//   d

  Material floorMaterial;
  floorMaterial.color = white;
  floorMaterial.ambient = vec3(0.5);
  floorMaterial.diffuse = vec3(0.44);
  floorMaterial.specular = vec3(0.06);
  floorMaterial.emission = vec3(0);
  floorMaterial.shininess = 10.f;

  Material leftWallMaterial;
  leftWallMaterial.color = red;
  leftWallMaterial.ambient = vec3(0.5);
  leftWallMaterial.diffuse = vec3(0.46);
  leftWallMaterial.specular = vec3(0.04);
  leftWallMaterial.emission = vec3(0);
  leftWallMaterial.shininess = 2.f;

  Material rightWallMaterial;
  rightWallMaterial.color = green;
  rightWallMaterial.ambient = vec3(0.5);
  rightWallMaterial.diffuse = vec3(0.46);
  rightWallMaterial.specular = vec3(0.04);
  rightWallMaterial.emission = vec3(0);
  rightWallMaterial.shininess = 2.f;

  Material ceilingMaterial;
  ceilingMaterial.color = white;
  ceilingMaterial.ambient = vec3(0.5);
  ceilingMaterial.diffuse = vec3(0.04);
  ceilingMaterial.specular = vec3(0.46);
  ceilingMaterial.emission = vec3(0);
  ceilingMaterial.shininess = 10.f;

  Material backWallMaterial;
  backWallMaterial.color = white;
  backWallMaterial.ambient = vec3(0.5);
  backWallMaterial.diffuse = vec3(0.44);
  backWallMaterial.specular = vec3(0.06);
  backWallMaterial.emission = vec3(0);
  backWallMaterial.shininess = 10.f;

  // ---------------------------------------------------------------------------
  // Sphere 1
  std::vector<Primitive *> sphere1Primitives;
  
  sphere1Primitives.push_back(new Sphere(vec4(-0.45, 0.6, 0.4, 1), 0.4f, sphereMaterial));
  scene.push_back(new Object(sphere1Primitives));

  // ---------------------------------------------------------------------------
  // Sphere 2
  std::vector<Primitive *> sphere2Primitives;
  sphere2Primitives.push_back(new Sphere(vec4(0.6, 0.6, -0.4, 1), 0.3f, sphereMaterial));
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
      new Triangle(Vertex(vec4(3 * L / 5, 0.99 * L, 2 * L / 5, 1)),
                   Vertex(vec4(2 * L / 5, 0.99 * L, 2 * L / 5, 1)),
                   Vertex(vec4(3 * L / 5, 0.99 * L, 3 * L / 5, 1)),
                   lightMaterial));
  lightPrimitives.push_back(
      new Triangle(Vertex(vec4(2 * L / 5, 0.99 * L, 2 * L / 5, 1)),
                   Vertex(vec4(2 * L / 5, 0.99 * L, 3 * L / 5, 1)),
                   Vertex(vec4(3 * L / 5, 0.99 * L, 3 * L / 5, 1)),
                   lightMaterial));
  scene.push_back(new Object(lightPrimitives));

  // Floor:
  std::vector<Primitive *> floorPrimitives;
  floorPrimitives.push_back(
      new Triangle(Vertex(C), Vertex(B), Vertex(A), floorMaterial));
  floorPrimitives.push_back(
      new Triangle(Vertex(C), Vertex(D), Vertex(B), floorMaterial));
  scene.push_back(new Object(floorPrimitives));

  // Left wall
  std::vector<Primitive *> leftWallPrimitives;
  leftWallPrimitives.push_back(
      new Triangle(Vertex(A), Vertex(E), Vertex(C), leftWallMaterial));
  leftWallPrimitives.push_back(
      new Triangle(Vertex(C), Vertex(E), Vertex(G), leftWallMaterial));
  scene.push_back(new Object(leftWallPrimitives));

  // Right wall
  std::vector<Primitive *> rightWallPrimitives;
  rightWallPrimitives.push_back(
      new Triangle(Vertex(F), Vertex(B), Vertex(D), rightWallMaterial));
  rightWallPrimitives.push_back(
      new Triangle(Vertex(H), Vertex(F), Vertex(D), rightWallMaterial));
  scene.push_back(new Object(rightWallPrimitives));

  // Ceiling
  std::vector<Primitive *> ceilingPrimitives;
  ceilingPrimitives.push_back(
      new Triangle(Vertex(E), Vertex(F), Vertex(G), ceilingMaterial));
  ceilingPrimitives.push_back(
      new Triangle(Vertex(F), Vertex(H), Vertex(G), ceilingMaterial));
  scene.push_back(new Object(ceilingPrimitives));

  // Back wall
  std::vector<Primitive *> backWallPrimitives;
  backWallPrimitives.push_back(
      new Triangle(Vertex(G), Vertex(D), Vertex(C), backWallMaterial));
  backWallPrimitives.push_back(
      new Triangle(Vertex(G), Vertex(H), Vertex(D), backWallMaterial));
  scene.push_back(new Object(backWallPrimitives));

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
    for (size_t j = 0; j < scene[i]->primitives.size(); j++) {
      Triangle *tri;
      if ((tri = dynamic_cast<Triangle *>(scene[i]->primitives[j]))) {
        tri->v0.position *= 2 / L;
        tri->v1.position *= 2 / L;
        tri->v2.position *= 2 / L;

        tri->v0.position -= vec4(1, 1, 1, 1);
        tri->v1.position -= vec4(1, 1, 1, 1);
        tri->v2.position -= vec4(1, 1, 1, 1);

        tri->v0.position.x *= -1;
        tri->v1.position.x *= -1;
        tri->v2.position.x *= -1;

        tri->v0.position.y *= -1;
        tri->v1.position.y *= -1;
        tri->v2.position.y *= -1;

        tri->v0.position.w = 1.0;
        tri->v1.position.w = 1.0;
        tri->v2.position.w = 1.0;

        tri->ComputeNormal();
      }
    }
  }
}
