/*
 * Several general-purpose methods for computing mesh info.
 *
 * Author: Jeroen Baert
 */

#ifndef MESH_INFO_H_
#define MESH_INFO_H_

#include "TriMesh.h"
#include "Model.h"
#include <vector>

void computeFaceNormals(const TriMesh* mesh, vector<vec> &facenormals);
float computeFeatureSize(const TriMesh* mesh);
bool to_camera(Model* m, int face, vec camera_position);

#endif /* MESH_INFO_H_ */
