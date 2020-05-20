/*
 * Implementation of the protected/public functions of the abstract class of LineDrawers, which
 * represents a subset of Drawers that solely draw lines.
 *
 *      Author: Jeroen Baert
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glui.h>
#include "LineDrawer.h"

/**
 * Protected LineDrawer constructor
 */
LineDrawer::LineDrawer(trimesh::vec color, float linewidth): Drawer(true), linecolor_(color), linewidth_(linewidth)
{

}

/**
 * Flush the drawbuffers to the OpenGL Draw buffer to display the computed lines, then clear them.
 */
void LineDrawer::flushDrawBuffer()
{
	// if we've got some lines to draw ...
	if(!drawbuffer_vertices_.empty()){
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(drawbuffer_vertices_[0]),&drawbuffer_vertices_[0][0]);
		// if per-line colors were defined
		if(!drawbuffer_colors_.empty()){
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, sizeof(drawbuffer_colors_[0]),&drawbuffer_colors_[0][0]);
		}
		// push lines to GPU
		glDrawArrays(GL_LINES, 0, drawbuffer_vertices_.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		// clear the buffers
		drawbuffer_vertices_.clear();
		drawbuffer_colors_.clear();
	}
}

/**
 * Returns the current line color
 */
trimesh::vec LineDrawer::getLineColor(){
	return linecolor_;
}

/**
 * Returns the current line width
 */
float LineDrawer::getLineWidth(){
	return linewidth_;
}

/**
 * Sets the current line color
 */
void LineDrawer::setLineColor(trimesh::vec color){
	this->linecolor_ = color;
}

/**
 * Sets the current line width
 */
void LineDrawer::setLineWidth(float width){
	this->linewidth_ = width;
}
