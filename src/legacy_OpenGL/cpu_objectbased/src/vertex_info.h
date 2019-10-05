/*
 * vertex_info.h
 *
 * 	The definition of several general-purpose methods to compute view-dependent per-vertex information.
 *
 *      Author: Jeroen Baert
 */

#ifndef VERTEX_INFO_H_
#define VERTEX_INFO_H_

#include "TriMesh.h"
#include "Model.h"
#include <vector>

void compute_ndotv(const TriMesh *mesh, const vec camera, vector<float> &ndtov);
void compute_CurvDerivatives(const TriMesh *mesh, const vec camera, vector<float> &kr, vector<float> &num, vector<float> &den, float sc_threshold);

#endif /* VERTEX_INFO_H_ */
