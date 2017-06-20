// Object Space Suggestive Contours vertex shader
// Jeroen Baert - www.forceflow.be

// IN: camera position (per render) 
uniform vec3 cam_pos;

// OUT: variables for fragment shader
varying float ndotv;
varying float t_kr;
varying float t_dwkr;
varying vec3 view;
varying vec3 w;

void main()
{
	vec3 pdir1 = gl_MultiTexCoord1.stp;
	vec3 pdir2 = gl_MultiTexCoord2.stp;
	float curv1 = gl_MultiTexCoord3.s;
	float curv2 = gl_MultiTexCoord4.s;
	vec4 dcurv = gl_MultiTexCoord5.stpq;
	
	// compute vector to cam
	view = cam_pos - vec3(gl_Vertex.x,gl_Vertex.y,gl_Vertex.z);
	
	// compute ndotv (and divide by view)
	ndotv = (1.0f / length(view)) * dot(gl_Normal,view);
	
	// optimalisation: if this vector points away from cam, don't even bother computing the rest.
	// the data will not be used in computing pixel color
	if(!(ndotv < 0.0f))
	{
		// compute kr
		w = normalize(view - gl_Normal * dot(view, gl_Normal));
  		float u = dot(w, pdir1);
  		float v = dot(w, pdir2);
  		float u2 = u*u;
    	float v2 = v*v;
  		t_kr = (curv1*u2) + (curv2*v2);
  		// and dwkr
  		float uv = u*v;
  		float dwII = (u2*u*dcurv.x) + (3.0*u*uv*dcurv.y) + (3.0*uv*v*dcurv.z) + (v*v2*dcurv.w);
  		// extra term due to second derivative
  		t_dwkr = dwII + 2.0 * curv1 * curv2 * ndotv/sqrt((1.0 - pow(ndotv, 2.0)));
  	}
  	
	// position transformation
	gl_Position = ftransform();
} 
