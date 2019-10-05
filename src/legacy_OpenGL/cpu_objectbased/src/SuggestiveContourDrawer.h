/*
 * SuggestiveContourDrawer.h
 *
 *      Author: Jeroen Baert
 */

#ifndef SUGGESTIVECONTOURDRAWER_H_
#define SUGGESTIVECONTOURDRAWER_H_

#include "LineDrawer.h"

class SuggestiveContourDrawer: public LineDrawer {
private:
	bool fading_;
	float sc_thresh_;
	void construct_sc_segments(Model *m, int vec0, int vec1, int vec2, float fade_factor);
	void find_sc_segments(Model* m, vec camera_position, float fade_factor);
public:
	SuggestiveContourDrawer(vec color,float linewidth, bool fade, float sc_thresh);
	virtual void draw(Model* m, vec camera_position);
	virtual void toggleFading();
	virtual bool isFaded();
};

#endif /* SUGGESTIVECONTOURDRAWER_H_ */
