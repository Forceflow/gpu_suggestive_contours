/*
 * FPSCounter.cpp
 *
 *  Created on: Mar 14, 2010
 *      Author: jeroen
 */

#include "FPSCounter.h"
#include "timestamp.h"
#include <iostream>
#include <string>

using std::string;
using std::cout;
using std::endl;

FPSCounter::FPSCounter()
{
	lasttime = now();
	FPS = 0;
	frames = 0;
}

void FPSCounter::updateCounter()
{
	if(now().tv_sec - lasttime.tv_sec >= 1)
	{
		FPS = frames;
		frames = 0;
	}
	lasttime = now();
	frames++;
}
