/*
 * LineDrawer.h
 *
 *      Author: Jeroen Baert
 */

#ifndef LINEDRAWER_H_
#define LINEDRAWER_H_

#include "Drawer.h"

class LineDrawer: public Drawer {

protected:
	// line properties
	vec linecolor_;
	float linewidth_;
	// buffers
	vector<vec> drawbuffer_vertices_;
	vector<vec4> drawbuffer_colors_;

	LineDrawer(vec color, float linewidth);
	void flushDrawBuffer();

public:
	vec getLineColor();
	float getLineWidth();
	void setLineColor(vec color);
	void setLineWidth(float width);
};

#endif /* LINEDRAWER_H_ */
