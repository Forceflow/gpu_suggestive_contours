// Object Space Suggestive Contours vertex shader
// Jeroen Baert - www.forceflow.be

// IN: computed values from vertex shader
	varying float ndotv;
	varying float t_kr;
	varying float t_dwkr;

// IN: uniform values
	uniform float fz;
	uniform float c_limit;
	uniform float sc_limit;
	uniform float dwkr_limit;

void main()
{
	// base color
	vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
	
	// use feature size
	float kr = fz*abs(t_kr); // absolute value to use it in limits
	float dwkr = fz*fz*t_dwkr; // two times fz because derivative
	float dwkr2 = (dwkr-dwkr*pow(ndotv, 2.0));

	// compute limits
	float contour_limit = c_limit*(pow(ndotv, 2.0)/kr);
	float sc_limit = sc_limit*(kr/dwkr2);
	// contours
	if(contour_limit<1.0)
	{color.xyz = min(color.xyz, vec3(contour_limit, contour_limit, contour_limit));}
	// suggestive contours
	else if((sc_limit<1.0) && dwkr2>dwkr_limit)
	{color.xyz = min(color.xyz, vec3(sc_limit, sc_limit, sc_limit));}
	gl_FragColor = color;
} 
