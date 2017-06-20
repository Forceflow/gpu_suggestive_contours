/*
 * FPSCounter.h
 *
 *  Created on: Mar 14, 2010
 *      Author: jeroen
 */

#include "timestamp.h"


#ifndef FPSCOUNTER_H_
#define FPSCOUNTER_H_

class FPSCounter
{
public:
	FPSCounter();
	void updateCounter();
	int FPS;
private:
	timestamp lasttime;					// Update Our Time Variable
	int frames;						// Save The FPS
};

#endif /* FPSCOUNTER_H_ */
