/*
 * model.h
 *
 * A class representing a worldspace model
 *
 *      Author: Jeroen Baert
 */

#include <TriMesh.h>
#include "Drawer.h"
#include <vector>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glui.h>

#ifndef MODEL_H_
#define MODEL_H_

class Drawer;

class Model
{
/**
 * Definition of the Model class, representing a loaded mesh and its view-independent and view-dependent data.
 *
 * Author: Jeroen Baert
 */

private:
	// some private helper functions
	void computeViewIndependentData();
	void clearViewDependentData();
	void setupVBOs();

public:

	// the mesh_ representing this model
	const TriMesh* mesh_;
	// the drawer stack
	vector<Drawer*> drawers_;

	// vertex buffer objects for GPU storage
	GLuint vbo_positions_;
	GLuint vbo_normals_;

	// VIEW INDEPENDENT VALUES
	vector<vec> facenormals_;
	float feature_size_;

	// VIEW_DEPENDENT VALUES
	vector<float> ndotv_; // ndotv_
	vector<float> kr_; // radial curvature
	vector<float> num_; // second derivative of radial curv
	vector<float> den_; // second derivative of radial curv

	// constructor
	Model(const char* filename);

	// draw the model
	void draw(vec camera_position);
	// pop a drawer from the drawer stack
	void popDrawer();
	// push a drawer into the drawer stack
	void pushDrawer(Drawer* d);
	// clear all drawers_ from the drawer stack
	void clearDrawers();

	// compute ndotv_ for all vertices in this model, given a camera position
	void needNdotV(vec camera_position);
	// compute all curvature derivatives in this model, given a camera position
	// and a threshold for small derivatives
	void needCurvDerivatives(vec camera_position, float sc_threshold);
};

#endif /* MODEL_H_ */
