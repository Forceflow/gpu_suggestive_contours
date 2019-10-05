gpu_suggestive_contours
=======================

Experiments in rendering Suggestive Contours

Video demonstrating the effect:

[![IMAGE ALT TEXT](http://img.youtube.com/vi/meI1kt2UQtQ/0.jpg)](http://www.youtube.com/watch?v=meI1kt2UQtQ)

## Legacy

The ``legacy_OpenGL`` directory contains experiments I did for my compsci [Master's thesis](https://www.forceflow.be/thesis/overview/) in 2009-2010. These are all based on legacy OpenGL (pre 3.x fixed function pipeline).
They are built on the mesh_viewer utility from [TriMesh2](https://gfx.cs.princeton.edu/proj/trimesh2/), version 2.9. Newer Trimesh2 versions (even my own [Trimesh2 fork](https://github.com/Forceflow/trimesh2)) will probably not work to get these projects up and running.

The variants I generated differ in the approach they take to generate contours and suggestive contours:

 - **cpu_objectspace:** Generation of contours multi-threaded on CPU. Drawn using GL_LINE. Not fast.
 - **gpu_objectbased:** A mix between pre-generating properties once on CPU and doing the per-frame calculations in a vertex + fragment shader. Faster.
 - **gpu_postprocess_sobel:** Postprocess generation of contours using a Sobel filter based on a depth map output. Fast, but not great quality: misses a lot of the finer details.
 - **gpu_postprocess_radial:** Postprocess generation of contours using a custom radial filter based on a diffuse-shaded render output. Fast, good quality.

## New (2019)

It's 2019 now, and we've got modern API's and flying cars (strikethrough where applicable).

I will be porting the suggestive contour effects to whatever platform I like, for fun and non-profit. I'm currently thinking about WebGL (using three.js) and maybe a ReShade version, to inject the effects into modern games.
