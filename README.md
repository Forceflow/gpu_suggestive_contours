gpu_suggestive_contours
=======================

Experiments in rendering Suggestive Contours

Video demonstrating the effect:

[![IMAGE ALT TEXT](http://img.youtube.com/vi/meI1kt2UQtQ/0.jpg)](http://www.youtube.com/watch?v=meI1kt2UQtQ)

## Legacy OpenGL work (2009-2010)

The ``legacy_OpenGL`` directory contains experiments I did for my compsci [Master's thesis](https://www.forceflow.be/thesis/overview/) in 2009-2010. These are all based on legacy OpenGL (pre 3.x fixed function pipeline).
They are built on the mesh_viewer utility from [TriMesh2](https://gfx.cs.princeton.edu/proj/trimesh2/)

Requirements for all
 * [Trimesh2](https://github.com/Forceflow/trimesh2): All my code back then was based on Trimesh2 2.9. There have been some major changes in that library, so unless otherwise noted, you need that exact version to attempt building the project.
 * GLEW
 * GLUT (I use FreeGlut)

The variants I generated differ in the approach they take to generate contours and suggestive contours:

 - **cpu_objectspace:** Generation of contours multi-threaded on CPU. Drawn using GL_LINE. Not fast.
   - In may 2020, I upgraded this code to run in a more modern environment. The conversion was quick and dirty, but at least it compiles and runs.
   - There is a recent MSVC project for this now: you can find it in the `legacy_OpenGL\cpu_objectbased\msvc` folder.
   - Updated to recent TriMesh version 2.16
   - GLEW gets statically linked
   - Only win32 supported, though there's no reason 64-bit shouldn't work, if you link to new versions of the libs
   - In a perfect world, this would be rewritten to use GLFW and fixed-function OpenGL calls
   - Configure your library locations in `custom_includes.prop`
 - **gpu_objectbased:** A mix between pre-generating properties once on CPU and doing the per-frame calculations in a vertex + fragment shader. Faster.
   - Untouched, needs TriMesh 2.9
 - **gpu_postprocess_sobel:** Postprocess generation of contours using a Sobel filter based on a depth map output. Fast, but not great quality: misses a lot of the finer details.
   - Untouched, needs TriMesh 2.9
 - **gpu_postprocess_radial:** Postprocess generation of contours using a custom radial filter based on a diffuse-shaded render output. Fast, good quality.
   - Untouched, needs TriMesh 2.9

## New (2019)

It's 2019 now, and we've got modern API's and flying cars (strikethrough where applicable).

I will be porting the suggestive contour effects to whatever platform I like, for fun and non-profit. I'm currently thinking about WebGL (using three.js) and maybe a ReShade version, to inject the effects into modern games.
