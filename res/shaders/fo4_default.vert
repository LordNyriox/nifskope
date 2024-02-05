#version 130

out vec3 LightDir;
out vec3 ViewDir;

out mat3 btnMatrix;

out vec4 A;
out vec4 C;
out vec4 D;


uniform bool isGPUSkinned;
uniform mat4 boneTransforms[100];

void main( void )
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec3 v;
	if ( !isGPUSkinned ) {
		btnMatrix[2] = normalize(gl_NormalMatrix * gl_Normal);
		btnMatrix[1] = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
		btnMatrix[0] = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);
		v = vec3(gl_ModelViewMatrix * gl_Vertex);
	} else {
		mat4 bt = boneTransforms[int(gl_MultiTexCoord3[0])] * gl_MultiTexCoord4[0];
		bt += boneTransforms[int(gl_MultiTexCoord3[1])] * gl_MultiTexCoord4[1];
		bt += boneTransforms[int(gl_MultiTexCoord3[2])] * gl_MultiTexCoord4[2];
		bt += boneTransforms[int(gl_MultiTexCoord3[3])] * gl_MultiTexCoord4[3];

		vec4 V = bt * gl_Vertex;
		vec3 normal = vec3(bt * vec4(gl_Normal, 0.0));
		vec3 tan = vec3(bt * vec4(gl_MultiTexCoord1.xyz, 0.0));
		vec3 bit = vec3(bt * vec4(gl_MultiTexCoord2.xyz, 0.0));

		gl_Position = gl_ModelViewProjectionMatrix * V;
		btnMatrix[2] = normalize(gl_NormalMatrix * normal);
		btnMatrix[1] = normalize(gl_NormalMatrix * tan);
		btnMatrix[0] = normalize(gl_NormalMatrix * bit);
		v = vec3(gl_ModelViewMatrix * V);
	}

	ViewDir = -v.xyz;
	LightDir = gl_LightSource[0].position.xyz;

	A = vec4(sqrt(gl_LightSource[0].ambient.rgb) * 0.375, gl_LightSource[0].ambient.a);
	C = gl_Color;
	D = sqrt(gl_LightSource[0].diffuse);
}
