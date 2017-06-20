/*
Szymon Rusinkiewicz
Princeton University

mesh_view.cc
Simple viewer

Extended for shader contours by Jeroen Baert
*/

#define GL_GLEXT_PROTOTYPES

// GL includes
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glui.h>
#include "glext.h"
// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <malloc.h>
#include <sstream>
// Own includes
#include "TriMesh.h"
#include "textfile.h"
#include "XForm.h"
#include "GLCamera.h"
#include "ICP.h"
#include "FPSCounter.h"

// Because we love strings
using std::string;
using std::cout;
using std::endl;

// Rendering parameters
int WIDTH = 512;
int RADIUS = 5;

// Global variables
vector<TriMesh *> meshes;
vector<xform> xforms;
vector<bool> visible;
vector<string> xffilenames;
TriMesh::BSphere global_bsph;
xform global_xf;
GLCamera camera;
vec camera_pos;
GLUI* glui_window;

// Program variables
int current_mesh = -1;
bool draw_lit = true; // always true, we need diffuse shaded object

// Textures
GLuint depth_texture;
GLuint color_texture;

// Vertex Buffer Objects
GLuint vbo_base;
GLuint vbo_normal;

// Shader programs
GLhandleARB shader_radial;
GLhandleARB shader_radial_vert;
GLhandleARB shader_radial_frag;

// Uniform locations
GLint loc_radius;
GLint loc_width;

// Performance counters
FPSCounter* fps;

// Make some mesh current
void set_current(int i)
{
	if (i >= 0 && i < meshes.size() && visible[i])
		current_mesh = i;
	else
		current_mesh = -1;
	camera.stopspin();
}

// Change visiblility of a mesh
void toggle_vis(int i)
{
	if (i >= 0 && i < meshes.size())
		visible[i] = !visible[i];
	if (current_mesh == i && !visible[i])
		set_current(-1);
}

// Signal a redraw
void need_redraw()
{
	glutPostRedisplay();
}

// Clear the screen
void cls()
{
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	glClearColor(1, 1, 1, 0);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

static int printOglError(char *file, int line)
{
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
        retCode = 1;
        glErr = glGetError();
    }
    return retCode;
}


static void printInfoLog(GLhandleARB obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
        free(infoLog);
    }
}

void setup_uniforms()
{
	glUniform1iARB(loc_radius, RADIUS);
	glUniform1iARB(loc_width, WIDTH);
}

// Set up lights and materials
void setup_lighting(int id)
{
	Color c(1.0f);
	glColor3fv(c);
	GLfloat light0_diffuse[] = { 0.85, 0.85, 0.85, 0.85 };
	GLfloat light0_position[] = { camera_pos[0], camera_pos[1], camera_pos[2], 0.0};
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);
}

// Draw triangle strips.  They are stored as length followed by values.
void draw_tstrips(const TriMesh *themesh)
{
	static bool use_glArrayElement = false;
	static bool tested_renderer = false;
	if (!tested_renderer) {
		use_glArrayElement = !!strstr(
			(const char *) glGetString(GL_RENDERER), "Intel");
		tested_renderer = true;
	}

	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();
	if (use_glArrayElement) {
		while (likely(t < end)) {
			glBegin(GL_TRIANGLE_STRIP);
			int striplen = *t++;
			for (int i = 0; i < striplen; i++)
				glArrayElement(*t++);
			glEnd();
		}
	} else {
		while (likely(t < end)) {
			int striplen = *t++;
			glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
			t += striplen;
		}
	}
}

// Draw the mesh
void draw_mesh(int i)
{
	const TriMesh *themesh = meshes[i];

	glPushMatrix();
	glMultMatrixd(xforms[i]);

	// we need the depth tests
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// Vertices (in VBO)
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_base);
	glEnableClientState(GL_VERTEX_ARRAY); // enable vertices
	glVertexPointer(3, GL_FLOAT,0,0);
	// Normals (in VBO)
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_normal);
	glEnableClientState(GL_NORMAL_ARRAY); // enable vertices
	glNormalPointer(GL_FLOAT,0,0);
	// switch off VBO's again
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	draw_tstrips(themesh);

	glPopMatrix();
}

/**
 * Draw a full size quad covering the whole screen, regardless of camera standpoint.
 * This quad has texture coords, so we can overlay a texture on it.
 */
void drawFullsizeQuad()
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glBegin(GL_QUADS);
	glMultiTexCoord2f(GL_TEXTURE0, 0.0, 0.0);	glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0);	glVertex3f(-1.0, -1.0, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0, 1.0, 0.0);	glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0);	glVertex3f(1.0, -1.0, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0, 1.0, 1.0);   glMultiTexCoord2f(GL_TEXTURE1, 1.0, 1.0);	glVertex3f(1.0, 1.0, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0, 0.0, 1.0);	glMultiTexCoord2f(GL_TEXTURE1, 0.0, 1.0);   glVertex3f(-1.0, 1.0, 0.0);
	glEnd();

	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// Draw the scene
void redraw()
{
	// position camera, push matrices, and clear screen
	camera.setupGL(global_xf * global_bsph.center, global_bsph.r);
	camera_pos = inv(global_xf) * point(0,0,0);
	glPushMatrix();
	glMultMatrixd(global_xf);
	cls();

	// draw the diffuse-shaded mesh to fill the color buffer
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		if (!visible[i]){continue;}
		setup_lighting(i);
		draw_mesh(i);
	}

	// copy rendered frame to color texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, WIDTH, WIDTH);

	// clear screen
	cls();
    // enable shader
    glUseProgramObjectARB(shader_radial);
    setup_uniforms();
	// draw a fullsize quad to force shader computation
	drawFullsizeQuad();
	// disable shader
	glUseProgramObjectARB(0);

	glPopMatrix();
	glutSwapBuffers();
	fflush(stdout);
	// update FPS counter
	fps->updateCounter();
	std::stringstream out;out << "NPR Thesis Jeroen Baert - GPU Imagespace | Pol: " << meshes[0]->faces.size() << " | FPS: " << fps->FPS;string s = out.str();
	glutSetWindowTitle(s.c_str());
}

void load_shader(GLhandleARB &program, GLhandleARB &vertex, GLhandleARB &frag, string vertex_filename, string frag_filename)
{
	// reusable char pointers to read files
	char *vs = NULL,*fs = NULL;
	// Create shader objects
	vertex = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	frag = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	// Read sources for these objects
	vs = textFileRead(vertex_filename.c_str()); const char * vv = vs; glShaderSourceARB(vertex, 1, &vv,NULL);
	fs = textFileRead(frag_filename.c_str()); const char * ff = fs; glShaderSourceARB(frag, 1, &ff,NULL);
	free(vs);free(fs);
	// Compile the shader objects and print info on compile
	glCompileShaderARB(vertex); printInfoLog(vertex);
	glCompileShaderARB(frag); printInfoLog(frag);
	// Create shader program object and attach vertex/frag objects
	program = glCreateProgramObjectARB();
	glAttachObjectARB(program,vertex);
	glAttachObjectARB(program,frag);
	// Link shader program and print info on linking
	glLinkProgramARB(program), printInfoLog(program);
}

void setup_shaders()
{
	// load Sobel shader
	load_shader(shader_radial,shader_radial_vert,shader_radial_frag,"radial.vert","radial.frag");

	// activate shader for texture binding
	glUseProgramObjectARB(shader_radial);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	GLint handle = glGetUniformLocationARB(shader_radial, "color");
	glUniform1iARB(handle, 0);

	// get locations of uniforms
	loc_radius = glGetUniformLocationARB(shader_radial, "radius");
	loc_width = glGetUniformLocationARB(shader_radial, "renderwidth");

	// disable shader
	glUseProgramObjectARB(0);
}

void setup_textures()
{
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, WIDTH, 0, GL_RGB, GL_FLOAT, NULL);
}

void setupVBOs()
{
	// generate a vertex buffer object
	glGenBuffersARB(1, &vbo_base);
	// bind it and specify kind of data
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_base);
	// upload data to VBO: we're sending data that will remain STATIC (base mesh is unchanged) and is used for DRAWing.
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->vertices.size()*sizeof(float)*3, &(meshes[0]->vertices[0]), GL_STATIC_DRAW_ARB);
	int bufferSize;
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "Vertex array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for normals
	glGenBuffersARB(1, &vbo_normal);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_normal);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->normals.size()*sizeof(float)*3, &(meshes[0]->normals[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "Normal array loaded in VBO: " << bufferSize << " bytes\n";

	// don't use any VBO right now, this would fudge with pointer arithmetic
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

// Update global bounding sphere.
void update_bsph()
{
	point boxmin(1e38, 1e38, 1e38);
	point boxmax(-1e38, -1e38, -1e38);
	bool some_vis = false;
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])	
			continue;
		some_vis = true;
		point c = xforms[i] * meshes[i]->bsphere.center;
		float r = meshes[i]->bsphere.r;
		for (int j = 0; j < 3; j++) {
			boxmin[j] = min(boxmin[j], c[j]-r);
			boxmax[j] = max(boxmax[j], c[j]+r);
		}
	}
	if (!some_vis)
		return;
	point &gc = global_bsph.center;
	float &gr = global_bsph.r;
	gc = 0.5f * (boxmin + boxmax);
	gr = 0.0f;
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])	
			continue;
		point c = xforms[i] * meshes[i]->bsphere.center;
		float r = meshes[i]->bsphere.r;
		gr = max(gr, dist(c, gc) + r);
	}
}

// Set the view...
void resetview()
{
	camera.stopspin();
	for (int i = 0; i < meshes.size(); i++)
		if (!xforms[i].read(xffilenames[i]))
			xforms[i] = xform();

	update_bsph();
	global_xf = xform::trans(0, 0, -5.0f * global_bsph.r) *
		    xform::trans(-global_bsph.center);

	// Special case for 1 mesh
	if (meshes.size() == 1 && xforms[0].read(xffilenames[0])) {
		global_xf = xforms[0];
		xforms[0] = xform();
		update_bsph();
	}
}

// Save the current image to a PPM file.
// Uses the next available filename matching filenamepattern
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
			swap(row1[j], row2[j]);
	}

	// Write out file
	fprintf(f, "P6\n#\n%d %d\n255\n", width, height);
	fwrite(buf, width*height*3, 1, f);
	fclose(f);
	delete [] buf;

	printf("Done.\n\n");
}

// Save all transforms
void save_xforms()
{
	if (xforms.size() == 1) {
		printf("Writing %s\n", xffilenames[0].c_str());
		global_xf.write(xffilenames[0]);
		return;
	}
	for (int i = 0; i < xforms.size(); i++) {
		printf("Writing %s\n", xffilenames[i].c_str());
		xforms[i].write(xffilenames[i]);
	}
}


// ICP
void do_icp(int n)
{
	camera.stopspin();

	if (current_mesh < 0 || current_mesh >= meshes.size())
		return;
	if (n < 0 || n >= meshes.size())
		return;
	if (!visible[n] || !visible[current_mesh] || n == current_mesh)
		return;
	ICP(meshes[n], meshes[current_mesh], xforms[n], xforms[current_mesh], 2);
	update_bsph();
	need_redraw();
}

// Handle mouse button and motion events
static unsigned buttonstate = 0;

void doubleclick(int button, int x, int y)
{
	// Render and read back ID reference image
	camera.setupGL(global_xf * global_bsph.center, global_bsph.r);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	glMultMatrixd(global_xf);
	for (int i = 0; i < meshes.size(); i++) {
		if (!visible[i])
			continue;
		glColor3ub((i >> 16) & 0xff,
			   (i >> 8)  & 0xff,
			    i        & 0xff);
		draw_mesh(i);
	}
	glPopMatrix();
	GLint V[4];
	glGetIntegerv(GL_VIEWPORT, V);
	y = int(V[1] + V[3]) - 1 - y;
	unsigned char pix[3];
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pix);
	int n = (pix[0] << 16) + (pix[1] << 8) + pix[2];

	if (button == 0 || buttonstate == (1 << 0)) {
		// Double left click - select a mesh
		set_current(n);
	} else if (button == 2 || buttonstate == (1 << 2)) {
		// Double right click - ICP current to clicked-on
		do_icp(n);
	}
}

void mousemotionfunc(int x, int y)
{
	static const Mouse::button physical_to_logical_map[] = {
		Mouse::NONE, Mouse::ROTATE, Mouse::MOVEXY, Mouse::MOVEZ,
		Mouse::MOVEZ, Mouse::MOVEXY, Mouse::MOVEXY, Mouse::MOVEXY,
	};

	Mouse::button b = Mouse::NONE;
	if (buttonstate & (1 << 3))
		b = Mouse::WHEELUP;
	else if (buttonstate & (1 << 4))
		b = Mouse::WHEELDOWN;
	else if (buttonstate & (1 << 30))
		b = Mouse::LIGHT;
	else
		b = physical_to_logical_map[buttonstate & 7];

	if (current_mesh < 0) {
		camera.mouse(x, y, b,
			     global_xf * global_bsph.center, global_bsph.r,
			     global_xf);
	} else {
		xform tmp_xf = global_xf * xforms[current_mesh];
		camera.mouse(x, y, b,
			     tmp_xf * meshes[current_mesh]->bsphere.center,
			     meshes[current_mesh]->bsphere.r,
			     tmp_xf);
		xforms[current_mesh] = inv(global_xf) * tmp_xf;
		update_bsph();
	}
	if (b != Mouse::NONE)
		need_redraw();
}

void mousebuttonfunc(int button, int state, int x, int y)
{
	static timestamp last_click_time;
	static unsigned last_click_buttonstate = 0;
	static float doubleclick_threshold = 0.25f;

	if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
		buttonstate |= (1 << 30);
	else
		buttonstate &= ~(1 << 30);

	if (state == GLUT_DOWN) {
		buttonstate |= (1 << button);
		if (buttonstate == last_click_buttonstate &&
		    now() - last_click_time < doubleclick_threshold) {
			doubleclick(button, x, y);
			last_click_buttonstate = 0;
		} else {
			last_click_time = now();
			last_click_buttonstate = buttonstate;
		}
	} else {
		buttonstate &= ~(1 << button);
	}

	mousemotionfunc(x, y);
}


// Idle callback
void idle()
{
	xform tmp_xf = global_xf;
	if (current_mesh >= 0)
		tmp_xf = global_xf * xforms[current_mesh];

	if (camera.autospin(tmp_xf))
		need_redraw();
	else
		usleep(10000);

	if (current_mesh >= 0) {
		xforms[current_mesh] = inv(global_xf) * tmp_xf;
		update_bsph();
	} else {
		global_xf = tmp_xf;
	}
}

// Keyboard
#define Ctrl (1-'a')
void keyboardfunc(unsigned char key, int x, int y)
{
	switch (key) {
		case ' ':
			if (current_mesh < 0)
				resetview();
			else
				set_current(-1);
			break;
		case 'I':
			dump_image(); break;
		case Ctrl+'x':
			save_xforms();
			break;
		case '\033': // Esc
		case Ctrl+'q':
		case 'Q':
		case 'q':
			exit(0);
		default:
			if (key >= '1' && key <= '9') {
				int m = key - '1';
				toggle_vis(m);
			}
	}
	need_redraw();
}

void usage(const char *myname)
{
	fprintf(stderr, "Usage: %s infile...\n", myname);
	exit(1);
}

int main(int argc, char *argv[])
{
	glutInitWindowSize(WIDTH, WIDTH);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInit(&argc, argv);

	if (argc < 2)
		usage(argv[0]);

	for (int i = 1; i < argc; i++) {
		const char *filename = argv[i];
		TriMesh *themesh = TriMesh::read(filename);
		if (!themesh)
			usage(argv[0]);
		themesh->need_normals();
		themesh->need_tstrips();
		themesh->need_bsphere();
		meshes.push_back(themesh);

		string xffilename = xfname(filename);
		xffilenames.push_back(xffilename);

		xforms.push_back(xform());
		visible.push_back(true);
	}

	glutCreateWindow(argv[1]);
	glutDisplayFunc(redraw);
	glutMouseFunc(mousebuttonfunc);
	glutMotionFunc(mousemotionfunc);
	glutKeyboardFunc(keyboardfunc);
	glutIdleFunc(idle);

	//  Create GLUI window
	glui_window = GLUI_Master.create_glui ("Options");
	glui_window->add_slider("Edge Detection Radius", GLUI_SLIDER_INT, 1, 15, &RADIUS);
	glui_window->sync_live();

	// init GLEW to use all the magic
	glewInit();

	// textures
	setup_textures();

	// shaders
	printf ("Checking for GLSL support ...");
	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
	{printf("OK\n");
	setup_shaders();}
	else{printf("No GLSL support. This application requires at least OpenGL 1.4 with ARB extensions.\n");exit(3);}

	// VBO
	printf ("Checking for Vertex Buffer Object support ...");
	if (GLEW_ARB_vertex_buffer_object)
	{printf(" OK\n");
	setupVBOs();}
	else{printf("No VBO support. This application requires at least OpenGL 1.4 with ARB extensions.\n");exit(3);}

	// Create FPS counter
	fps = new FPSCounter();
	camera_pos = inv(global_xf) * point(0,0,0);
	resetview();
	glutMainLoop();
}
