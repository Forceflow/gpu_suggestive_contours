// Contours vertex shader
// Jeroen Baert
// www.forceflow.be

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	// position transformation
	gl_Position = ftransform();
} 
