/*
 * FPSCounter.h
 *
 *	Definition of an easy and slightly inaccurate FPS counter
 *
 *  Created on: Mar 14, 2010
 *      Author: jeroen
 */

#include "timestamp.h"


#ifndef FPSCOUNTER_H_
#define FPSCOUNTER_H_

class FPSCounter{
private:
    trimesh::timestamp lasttime;
	int frames;
public:
	FPSCounter();
	void updateCounter();
	int FPS;
};

#endif /* FPSCOUNTER_H_ */
