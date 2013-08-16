// Object Space Suggestive Contours vertex shader
// Jeroen Baert - www.forceflow.be

// IN: computed values from vertex shader
	varying float ndotv;
	varying float t_kr;
	varying float t_dwkr;
	varying vec3 view;
    varying vec3 w;

// IN: uniform values
    uniform bool jeroenmethod;
	uniform float fz;
	uniform float c_limit;
	uniform float sc_limit;
	uniform float dwkr_limit;

void main()
{
	// base color
	vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
	
	// use feature size
	float kr = fz*t_kr; // absolute value to use it in limits
	float dwkr = fz*fz*t_dwkr; // two times fz because derivative
	float dwkr_theta = (dwkr-dwkr*pow(ndotv, 2.0))/ length(w);

	// compute limits
	float contour_limit = c_limit*(pow(ndotv, 2.0)/abs(kr));
	
	float suggestive_contour_limit;
	if(jeroenmethod){
		suggestive_contour_limit = sc_limit*(abs(kr)/dwkr_theta);
	} else {
		suggestive_contour_limit = sc_limit*(dwkr);
	}
	
	
	
	// contours
	if(contour_limit<1.0)
	{color.xyz = min(color.xyz, vec3(0, 0, 0));}
	// suggestive contours
	else if((suggestive_contour_limit<1.0) && dwkr_theta>dwkr_limit)
	{color.xyz = min(color.xyz, vec3(0, 0, 0));}
	gl_FragColor = color;
} 
