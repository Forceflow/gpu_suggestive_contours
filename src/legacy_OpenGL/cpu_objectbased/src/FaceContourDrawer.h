/*
 * Definition of a FaceContourDrawer, which is a LineDrawer that draws contour lines on the faces of a given model,
 * using lineair interpolation between the zero points of n dot v.
 *
 *      Author: Jeroen Baert
 */

#ifndef FACECONTOURDRAWER_H_
#define FACECONTOURDRAWER_H_

#include "LineDrawer.h"

class FaceContourDrawer: public LineDrawer{
private:
	void construct_faceline(Model* m,int v0, int v1, int v2);
	void find_facelines(Model* m, vec camera_position);
public:
	FaceContourDrawer(vec color,float linewidth);
	virtual void draw(Model* m, vec camera_position);
};

#endif /* FACECONTOURDRAWER_H_ */
