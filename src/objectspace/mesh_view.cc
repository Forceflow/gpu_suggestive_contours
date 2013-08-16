/**
 * mesh_viewer
 * Original by Szymon Rusinkiewicz
 *
 * Modified for suggestive contour rendering with shaders by Jeroen Baert
 * www.forceflow.be
 */

 #define GLEW_STATIC

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <malloc.h>
#include "TriMesh.h"
#include "XForm.h"
#include "GLCamera.h"
#include "ICP.h"
#include "textfile.h"
#include <sstream>
#include "time.h"

using std::string;
using std::cout;
using std::endl;

using namespace std;

struct Timer {
	clock_t Begin;
	Timer(){
		Begin = clock() * CLK_TCK;
	}
	void reset(){
		Begin = clock() * CLK_TCK;
	}
	double getTimeMilliseconds(){
		clock_t End = clock() * CLK_TCK;
		return (End - Begin)/1000;
	}
};

// Render params
float c_limit = 1.0;
float sc_limit = 1.0;
float dwkr_limit = 0.05;
bool jeroenmethod = true;

// Globals
vector<TriMesh *> meshes;
vector<xform> xforms;
vector<bool> visible;
vector<string> xffilenames;
TriMesh::BSphere global_bsph;
xform global_xf;
GLCamera camera;
GLUI* glui_window;

//camera position
vec camera_pos;
int current_mesh = -1;
bool draw_lit = true;
bool draw_falsecolor = false;
bool draw_index = false;
bool white_bg = true;

// feature size
float feature_size;

// shader globals
GLhandleARB v,f,p;

// shader locations
GLint loc_cam;
GLint loc_fz;
GLint loc_c_limit;
GLint loc_sc_limit;
GLint loc_dwkr_limit;
GLint loc_jeroenmethod;

// vertex buffer objects for vertex data
GLuint vbo_base;
GLuint vbo_normal;
GLuint vbo_pdir1;
GLuint vbo_pdir2;
GLuint vbo_curv1;
GLuint vbo_curv2;
GLuint vbo_dcurv;

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

int printOglError(char *file, int line)
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


void printInfoLog(GLhandleARB obj, bool newline)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
                                         &infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
		if(newline){printf("%s\n",infoLog);}
		else{printf("%s",infoLog);}
        free(infoLog);
    }
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
	glCompileShaderARB(vertex); printInfoLog(vertex,false);
	glCompileShaderARB(frag); printInfoLog(frag,true);
	// Create shader program object and attach vertex/frag objects
	program = glCreateProgramObjectARB();
	glAttachObjectARB(program,vertex);
	glAttachObjectARB(program,frag);
	// Link shader program and print info on linking
	glLinkProgramARB(program), printInfoLog(program,false);
}

void setup_shaders()
{
	load_shader(p,v,f,"sc.vert","sc.frag");
	glUseProgramObjectARB(p);
	// get var locations once and for all
	loc_cam = glGetUniformLocationARB(p,"cam_pos");
	loc_fz = glGetUniformLocationARB(p,"fz");
	loc_c_limit = glGetUniformLocationARB(p,"c_limit");
	loc_sc_limit = glGetUniformLocationARB(p,"sc_limit");
	loc_dwkr_limit = glGetUniformLocationARB(p,"dwkr_limit");
	loc_jeroenmethod = glGetUniformLocationARB(p,"jeroenmethod");
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
	if (white_bg)
		glClearColor(1, 1, 1, 0);
	else
		glClearColor(0.08, 0.08, 0.08, 0);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Draw triangle strips.  They are stored as length followed by values.
void draw_tstrips(const TriMesh *themesh)
{
	// testing for glArrayElement support
	static bool use_glArrayElement = false;
	static bool tested_renderer = false;
	if (!tested_renderer) {use_glArrayElement = !!strstr((const char *) glGetString(GL_RENDERER), "Intel");
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

	// Matrix pushing for viewport
	glPushMatrix();
	glMultMatrixd(xforms[i]);

	// setup depth functions
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// setup uniform variables (these are the same for every vertex)
	camera_pos = inv(global_xf) * point(0,0,0);
	glUniform3fARB(loc_cam, camera_pos[0], camera_pos[1], camera_pos[2]);
	glUniform1fARB(loc_fz, feature_size);
	glUniform1fARB(loc_c_limit, c_limit);
	glUniform1fARB(loc_sc_limit, sc_limit);
	glUniform1fARB(loc_dwkr_limit, dwkr_limit);
	glUniform1fARB(loc_jeroenmethod, jeroenmethod);

	// Vertices
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_base);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT,0,0);
	// Normals
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_normal);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,0,0);
	// setup texture coords
	glClientActiveTexture(GL_TEXTURE1);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_pdir1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3,GL_FLOAT, 0,0);

	glClientActiveTexture(GL_TEXTURE2);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_pdir2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3,GL_FLOAT, 0,0);

	glClientActiveTexture(GL_TEXTURE3);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_curv1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(1,GL_FLOAT, 0,0);

	glClientActiveTexture(GL_TEXTURE4);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_curv2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(1,GL_FLOAT, 0,0);

	glClientActiveTexture(GL_TEXTURE5);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_dcurv);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(4,GL_FLOAT, 0,0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// disable unnecessary stuff and then draw
	glDisable(GL_LIGHTING);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisable(GL_COLOR_MATERIAL);
	draw_tstrips(themesh);

	glPopMatrix();
}

// Draw the scene
void redraw()
{
    Timer t = Timer();
	camera.setupGL(global_xf * global_bsph.center, global_bsph.r);
	glPushMatrix();
	glMultMatrixd(global_xf);
	cls();
	for (int i = 0; i < meshes.size(); i++)
	{
		if (!visible[i])
			continue;
		draw_mesh(i);
	}
	glPopMatrix();
	glutSwapBuffers();
	fflush(stdout);

	std::stringstream out;
	out << "NPR Thesis Jeroen Baert | Polys: " << meshes[0]->faces.size() << " | FPS: " << 1000.0f / t.getTimeMilliseconds();
	string s = out.str();
	glutSetWindowTitle(s.c_str());
}

// Update global bounding sphere.
void update_bsph()
{
	point boxmin(1e38, 1e38, 1e38);
	point boxmax(-1e38, -1e38, -1e38);
	bool some_vis = false;
	for (unsigned int i = 0; i < meshes.size(); i++) {
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
	for (unsigned int i = 0; i < meshes.size(); i++) {
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
	draw_index = true;
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
	draw_index = false;
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
        case 'j':
			jeroenmethod = !jeroenmethod;
			cout << "jeroenmethod " << jeroenmethod << endl;
			break;
		case 'f':
			draw_falsecolor = !draw_falsecolor; break;
		case 'l':
			draw_lit = !draw_lit; break;
		case 'w':
			white_bg = !white_bg; break;
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

void setupVBOs()
{
	int bufferSize;

	// generate a vertex buffer object
	glGenBuffersARB(1, &vbo_base);
	// bind it and specify kind of data
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_base);
	// upload data to VBO: we're sending data that will remain STATIC (base mesh is unchanged) and is used for DRAWing.
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->vertices.size()*sizeof(float)*3, &(meshes[0]->vertices[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "Vertex array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for normals
	glGenBuffersARB(1, &vbo_normal);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_normal);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->normals.size()*sizeof(float)*3, &(meshes[0]->normals[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "Normal array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for pdir1
	glGenBuffersARB(1, &vbo_pdir1);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_pdir1);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->pdir1.size()*sizeof(float)*3, &(meshes[0]->pdir1[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "PDIR1 array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for pdir2
	glGenBuffersARB(1, &vbo_pdir2);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_pdir2);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->pdir2.size()*sizeof(float)*3, &(meshes[0]->pdir2[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "PDIR2 array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for curv1
	glGenBuffersARB(1, &vbo_curv1);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_curv1);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->curv1.size()*sizeof(float), &(meshes[0]->curv1[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "CURV1 array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for curv2
	glGenBuffersARB(1, &vbo_curv2);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_curv2);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->curv2.size()*sizeof(float), &(meshes[0]->curv2[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "CURV2 array loaded in VBO: " << bufferSize << " bytes\n";

	// do the same for dcurv
	glGenBuffersARB(1, &vbo_dcurv);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_dcurv);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, meshes[0]->dcurv.size()*sizeof(float)*4, &(meshes[0]->dcurv[0]), GL_STATIC_DRAW_ARB);
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    std::cout << "DCURV array loaded in VBO: " << bufferSize << " bytes\n";

	// disable VBO's to not disturb pointer arithmetic
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

int main(int argc, char *argv[])
{
	glutInitWindowSize(720, 720);
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
		themesh->need_curvatures();
		themesh->need_dcurv();
		feature_size = themesh->feature_size();
		meshes.push_back(themesh);

		string xffilename = xfname(filename);
		xffilenames.push_back(xffilename);

		xforms.push_back(xform());
		visible.push_back(true);
	}


	glutCreateWindow("Thesis Shader viewer");
	glutDisplayFunc(redraw);
	glutMouseFunc(mousebuttonfunc);
	glutMotionFunc(mousemotionfunc);
	glutKeyboardFunc(keyboardfunc);
	glutIdleFunc(idle);

	//  Create GLUI window
	glui_window = GLUI_Master.create_glui ("Options");
	glui_window->add_slider("Contour limit", GLUI_SLIDER_FLOAT, 0, 10, &c_limit);
	glui_window->add_slider("Suggestive Contour limit", GLUI_SLIDER_FLOAT, 0, 10, &sc_limit);
	glui_window->add_slider("DwKr limit", GLUI_SLIDER_FLOAT, 0, 0.5, &dwkr_limit);
	glui_window->sync_live();

	glewInit();

	// setup shaders
	printf ("Checking for GLSL support ...");
	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
	{printf("OK\n");
	setup_shaders();}
	else{printf("No GLSL support. This application requires at least OpenGL 1.4 with ARB extensions.\n");exit(3);}

	// setup VBO
	printf ("Checking for Vertex Buffer Object support ...");
	if (GLEW_ARB_vertex_buffer_object)
	{printf(" OK\n");
	setupVBOs();}
	else{printf("No VBO support. This application requires at least OpenGL 1.4 with ARB extensions.\n");exit(3);}

	// setup Multitexture
	printf ("Checking for Multitexture support ...");
	if (GLEW_ARB_multitexture)
	{printf(" OK\n");}
	else{printf("No Multitexture support. This application requires at least OpenGL 1.4 with ARB extensions.\n");exit(3);}

	// check amount of textures
	GLint iUnits; glGetIntegerv(GL_MAX_TEXTURE_UNITS, &iUnits);
	cout << "Amount of texture units available: " << iUnits;

	resetview();
	glutMainLoop();
}


