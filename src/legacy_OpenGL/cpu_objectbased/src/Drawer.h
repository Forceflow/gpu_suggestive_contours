/*
 * A virtual class defining a Drawer class, which is an abstract class defining a component
 * that can draw something on the screen, given a certain Model.
 *
 * In its current form, it only forces Drawers to supply a draw method which takes a model and a camera position.
 *
 *      Author: Jeroen Baert
 */

#ifndef DRAWER_H_
#define DRAWER_H_

#include "Model.h"

class Drawer{
protected:
	bool visible_;
	Drawer(bool isvisible);
public:
	virtual void draw(Model* m, vec camera_position) = 0;
	void toggleVisibility();
	bool isVisible();
};

#endif /* DRAWER_H_ */
