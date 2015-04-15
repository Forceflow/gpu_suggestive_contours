uniform sampler2D edge;
uniform float limit;
uniform int width;

float intensity(in vec4 color)
{
	return sqrt((color.x*color.x)+(color.y*color.y)+(color.z*color.z));
}

vec3 sobel(float step, vec2 center)
{
	// get samples around pixel
    float tleft = intensity(texture2D(edge,center + vec2(-step,step)));
    float left = intensity(texture2D(edge,center + vec2(-step,0)));
    float bleft = intensity(texture2D(edge,center + vec2(-step,-step)));
    float top = intensity(texture2D(edge,center + vec2(0,step)));
    float bottom = intensity(texture2D(edge,center + vec2(0,-step)));
    float tright = intensity(texture2D(edge,center + vec2(step,step)));
    float right = intensity(texture2D(edge,center + vec2(step,0)));
    float bright = intensity(texture2D(edge,center + vec2(step,-step)));

	// Sobel masks (to estimate gradient)
	//        1 0 -1     -1 -2 -1
	//    X = 2 0 -2  Y = 0  0  0
	//        1 0 -1      1  2  1

	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;
	float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;
    float color = sqrt((x*x) + (y*y));
    if (color > limit){return vec3(0.0,0.0,0.0);}
    return vec3(1.0,1.0,1.0);
 }

void main(void)
{
	gl_FragColor = vec4(1,1,1,0);
	float step = 1.0/512.0;
	vec2 center = gl_TexCoord[0].st;
	gl_FragColor.xyz = sobel(step,center);
}

