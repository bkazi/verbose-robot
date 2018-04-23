- [ ] use shape classes from raytracer
- [ ] use obj loader from raytracer
- [x] shadows
- [x] clipping
- [x] barycentric coordinates
- [x] issue when Z > -1.000
- [ ] Change the pipeline. Currently goes through the entire pipeline for each triangle in scene.
Need to change it so that it is more "sequential".
Should go Polygons -> Vertex Shader -> Vertices -> Clipping, Interpolation, etc -> Pixels -> Pixel Shader. Allows for obvious for loops over the entire screen space which means $$$ on GPU
