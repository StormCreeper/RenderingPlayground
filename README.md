# Toy Renderer

I started this project for a Master 2 image synthesis class, and I spent a lot of time extending it to experiment with different rendering techniques: raytracing, rasterization, PBR, ...

## Setting up (git + cmake) 

`git clone https://github.com/StormCreeper/RenderingPlayground.git`  
`cd RenderingPlayground`  
`mkdir build & cd build`  
`cmake -DCMAKE_BUILD_TYPE=Release ..`  
`cmake --build .`  
`./ToyRenderer.exe`  

## Features
### Editor
- Scene editor: transforms, material
- Lights editor: positions, types, attributes
- Rendering parameters: color correction, number of ray bounces
- Debug editor: show lights, show BVH, rebuild
### GPU Rasterizer
- PBR Point lights and materials
- Textures, materials
- Displacement mapping, normal mapping
### GPU Raytracer
- PBR Point lights and materials
- Recursive reflected and refracted rays
- BVH acceleration structure
- Textures, materials
### CPU Raytracer (deprecated)
- PBR Point lights and materials
- BVH acceleration structure

## Architecture
*Todo*

## Future work
- Displacement mapping in the raytracer (Pre-pass in compute shader ? Tesselation-less displacement ?)
- Correct UV mapping and normal mapping
- Gizmos
