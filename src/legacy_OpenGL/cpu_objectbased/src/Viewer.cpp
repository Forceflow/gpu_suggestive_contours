/**
 * A simple GLUT-based OpenGL viewer.
 * (Based on the TriMesh2 library code where indicated)
 * author: Jeroen Baert
 */

#define GLEW_STATIC

// GLEW
#include <GL/glew.h> // OpenGL Extension Wrangler functions
// FROM OS
#include <GL/gl.h> // OpenGL functions itself

// TRIMESH2
#include <TriMesh.h>
#include <XForm.h>
#include <GLCamera.h>
#include <GL/glut.h> // FreeGlut window management

#include <string>
#include <sstream>
#include <algorithm>


// SELFMADE
#include "Model.h"
#include "BaseDrawer.h"
#include "EdgeContourDrawer.h"
#include "FaceContourDrawer.h"
#include "SuggestiveContourDrawer.h"
#include "FPSCounter.h"

using std::string;

// Global variables (if this Viewer were a class, this would be its attributes)
std::vector<Model*> models; // the model list
std::vector<trimesh::xform> transformations; // model transformations
trimesh::TriMesh::BSphere global_bsph; // global boundingbox
trimesh::xform global_transf; // global transformations
trimesh::GLCamera camera; // global camera

// our fps counter
FPSCounter* fps;

// The drawers we'll use in this demo
BaseDrawer* b;
EdgeContourDrawer* b1;
SuggestiveContourDrawer* b2;

// toggle for diffuse lighting
bool diffuse = false;

/**
 * Clears the OpenGL Draw and Depth buffer, resets all relevant OpenGL states
 */
void cls(){
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	glClearColor(1, 1, 1, 0);;
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * Update the global bounding sphere
 * (from TriMesh2 library)
 */
void update_boundingsphere(){
	// largest box possible
	trimesh::point boxmin(1e38, 1e38, 1e38);
	trimesh::point boxmax(-1e38, -1e38, -1e38);
	// find outer coords
	for (unsigned int i = 0; i < models.size(); i++){
		trimesh::point c = transformations[i] * models[i]->mesh_->bsphere.center;
		float r = models[i]->mesh_->bsphere.r;
		for (int j = 0; j < 3; j++) {
			boxmin[j] = std::min(boxmin[j], c[j]-r);
			boxmax[j] = std::max(boxmax[j], c[j]+r);
		}
	}
	trimesh::point &gc = global_bsph.center;
	float &gr = global_bsph.r;
	gc = 0.5f * (boxmin + boxmax);
	gr = 0.0f;
	// find largest possible radius for sphere
	for (unsigned int i = 0; i < models.size(); i++) {
		trimesh::point c = transformations[i] * models[i]->mesh_->bsphere.center;
		float r = models[i]->mesh_->bsphere.r;
		gr = std::max(gr, dist(c, gc) + r);
	}
}

/**
 * Reset the current view: undo all camera transformations.
 */
void resetview()
{
	// kill the cam
	camera.stopspin();
	// undo all model transformations
	for (unsigned int i = 0; i < models.size(); i++){
		transformations[i] = trimesh::xform();
	}
	// recompute bounding sphere
	update_boundingsphere();
	// put ourselves in middle
	global_transf = trimesh::xform::trans(0, 0, -5.0f * global_bsph.r)* trimesh::xform::trans(-global_bsph.center);
}

/**
 * Setup the OpenGL lighting
 */
void setup_lighting(){
	if(!diffuse){
		trimesh::Color c(1.0f);
		glColor3fv(c);
		glDisable(GL_LIGHTING);
	}
	else{
		GLfloat light0_diffuse[] = { 0.85, 0.85, 0.85, 0.85 };
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_NORMALIZE);
	}
}

/**
 * Reposition the camera and draw every model in the scene.
 */
void redraw(){
	// setup camera and push global transformations
	camera.setupGL(global_transf * global_bsph.center, global_bsph.r);
	glPushMatrix();
	glMultMatrixd(global_transf);
	cls();

	// enable depth checking and backface culling
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// compute new camera position
	trimesh::vec camera_pos = inv(global_transf) * trimesh::point(0,0,0);

	// setup lighting
	setup_lighting();

	// draw every model
	for (unsigned int i = 0; i < models.size(); i++){
		// push model-specific transformations
		glPushMatrix();
		glMultMatrixd(transformations[i]);
		// tell model to execute its drawer stack
		models[i]->draw(camera_pos);
		// pop again
		glPopMatrix();
	}
	// pop global transformations
	glPopMatrix();
	glutSwapBuffers();
	// update FPS counter
	fps->updateCounter();
	std::stringstream out;
	out << "Crytek Object Space Contours Demo | FPS: " << fps->FPS;
	string s = out.str();
	glutSetWindowTitle(s.c_str());
}

/**
 * Save the current image to a PPM file.
 * (from TriMesh2 library)
 */
void dump_image()
{
	// Find first non-used filename
	const char filenamepattern[] = "img%d.ppm";
	int imgnum = 0;
	FILE *f;
	while (1) {
		char filename[1024];
		sprintf(filename, filenamepattern, imgnum++);
		f = fopen(filename, "rb");
		if (!f) {
			f = fopen(filename, "wb");
			printf("\n\nSaving image %s... ", filename);
			fflush(stdout);
			break;
		}
		fclose(f);
	}

	// Read pixels
	GLint V[4];
	glGetIntegerv(GL_VIEWPORT, V);
	GLint width = V[2], height = V[3];
	char *buf = new char[width*height*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(V[0], V[1], width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

	// Flip top-to-bottom
	for (int i = 0; i < height/2; i++) {
		char *row1 = buf + 3 * width * i;
		char *row2 = buf + 3 * width * (height - 1 - i);
		for (int j = 0; j < 3 * width; j++)
			std::swap(row1[j], row2[j]);
	}

	// Write out file
	fprintf(f, "P6\n#\n%d %d\n255\n", width, height);
	fwrite(buf, width*height*3, 1, f);
	fclose(f);
	delete [] buf;

	printf("Done.\n\n");
}

static unsigned buttonstate = 0;

/**
 * Handle mouse motions
 * (from TriMesh2 library)
 */
void mousemotionfunc(int x, int y){
	static const trimesh::Mouse::button map[] = {
			trimesh::Mouse::NONE, trimesh::Mouse::ROTATE, trimesh::Mouse::MOVEXY, trimesh::Mouse::MOVEZ,
			trimesh::Mouse::MOVEZ, trimesh::Mouse::MOVEXY, trimesh::Mouse::MOVEXY, trimesh::Mouse::MOVEXY,
	};

	// find out what exactly happened
	trimesh::Mouse::button b = trimesh::Mouse::NONE;
	if (buttonstate & (1 << 3))
		b = trimesh::Mouse::WHEELUP;
	else if (buttonstate & (1 << 4))
		b = trimesh::Mouse::WHEELDOWN;
	else if (buttonstate & (1 << 30))
		b = trimesh::Mouse::LIGHT;
	else // hmm, it was something else
		b = map[buttonstate & 7];

	// pass mouse movement to camera
	camera.mouse(x, y, b,global_transf * global_bsph.center, global_bsph.r,global_transf);

	// if we identified something as mouse movement, force redisplay
	if (b != trimesh::Mouse::NONE)
		glutPostRedisplay();
}

/**
 * Handle mouse button clicks
 * (from TriMesh2 library)
 */
void mousebuttonfunc(int button, int state, int x, int y){
	static trimesh::timestamp last_click_time;
	static unsigned last_click_buttonstate = 0;
	if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
		buttonstate |= (1 << 30);
	else
		buttonstate &= ~(1 << 30);

	if (state == GLUT_DOWN) {
		buttonstate |= (1 << button);
		last_click_time = trimesh::now();
		last_click_buttonstate = buttonstate;
	}
	else {
		buttonstate &= ~(1 << button);
	}
	mousemotionfunc(x, y);
}

/**
 * Handle keyboard events to toggle some functionalities in the drawers, for demonstration purposes in this sample
 */
void keyboardfunc(unsigned char key, int x, int y){
	switch (key) {
	case 'a': // toggle basedrawer
		b->toggleVisibility();
		printf ("Toggled Base Drawer Visiblity to %i \n", b->isVisible());
		break;
	case 'z': // toggle contourdrawer
		b1->toggleVisibility();
		printf ("Toggled Contour Drawer Visiblity to %i \n", b1->isVisible());
		break;
	case 'e': // toggle suggestive contour drawer
		b2->toggleVisibility();
		printf ("Toggled Suggestive Contour Drawer Visibility to %i \n", b2->isVisible());
		break;
	case 'f': // toggle suggestive contour fading
		b2->toggleFading();
		printf ("Toggled Suggestive Contour fading to %i \n", b2->isFaded());
		break;
	case 'g': // toggle colored lines
		b2->setLineColor(trimesh::Color(1.0,0.0,0.0));
		printf ("Suggestive Contour Lines in false color \n");
		break;
	case 'h': // toggle colored lines
		b2->setLineColor(trimesh::Color(0.0,0.0,0.0));
		printf ("Suggestive Contour Lines in black color \n");
		break;
	case 'd': // toggle diffuse lighting
		diffuse = !diffuse;
		printf ("Toggled diffuse lighting to %i \n", diffuse);
		break;
	case 'w': // dump image to file
		dump_image();
		break;
	}
	glutPostOverlayRedisplay();
}

/**
 * GLUT idle callback
 * (from TriMesh2 library)
 */
void idle(){
	trimesh::xform tmp_xf = global_transf;
	if (camera.autospin(tmp_xf)) // if the camera is still spinning
		glutPostRedisplay();
	else
		trimesh::usleep(10000); // do nothing
	global_transf = tmp_xf;

}

int main(int argc, char *argv[]){
	// Initialize GLUT window manager
	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInit(&argc, argv);
	glutCreateWindow("Crytek Object Space Contours Demo");
	glutDisplayFunc(redraw);
	glutKeyboardFunc(keyboardfunc);
	glutMouseFunc(mousebuttonfunc);
	glutMotionFunc(mousemotionfunc);
	glutIdleFunc(idle);
	glewInit();

	// Check for Vertex Buffer Object Support
	printf ("Checking for Vertex Buffer Object support ...");
	if (GLEW_ARB_vertex_buffer_object){
		printf(" OK\n");
	}
	else{
		printf("No VBO support. This application requires at least OpenGL 1.4 with ARB extensions.\n");
		exit(3);
	}

	// construct the Drawers we'll use in this demo
	b = new BaseDrawer();
	b1 = new EdgeContourDrawer(trimesh::vec(0,0,0),3.0);
    b2 = new SuggestiveContourDrawer(trimesh::vec(0,0,0), 2.0, true, 0.001);

    if (argc < 2){
    	printf("No models supplied. Please supply one or more OBJ/PLY models. \n");
    	exit(3);
    }

	// read models from arguments
	for (int i = 1; i < argc; i++){
		const char *name = argv[i];
		// creat model
		Model* m = new Model(name);
		// add drawers to model
		m->pushDrawer(b);
		m->pushDrawer(b1);
		m->pushDrawer(b2);
		models.push_back(m);
		// push back blank tranformation matrix
		transformations.push_back(trimesh::xform());
	}

	// create fps counter
	fps = new FPSCounter();

	// reset window viewpoint and start GLUT main loop (will never stop)
	resetview();
	glutMainLoop();
}

