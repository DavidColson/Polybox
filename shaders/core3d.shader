@ctype mat4 Matrixf 
@ctype vec4 Vec4f 
@ctype vec3 Vec3f 

@vs vs
uniform vs_params {
	mat4 mvp;
	mat4 model;
	int u_lightingEnabled;
	vec4 u_lightDirection[3];
	vec4 u_lightColor[3];
	vec3 u_lightAmbient;
};

layout(location=0) in vec3 pos;
layout(location=1) in vec4 color0;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;

noperspective out vec4 color;

void main() {
    gl_Position = mvp * vec4(pos, 1.0);
	vec3 norm = (model * vec4(normal, 0.0)).xyz;
    color = color0;

	if (u_lightingEnabled == 0)
	{
		color = color0;
	}
	else if (u_lightingEnabled == 1)
	{
		float lightMag = max(dot(normalize(u_lightDirection[0].xyz), norm.xyz), 0.0);
		vec3 diffuse = lightMag * u_lightColor[0].xyz;

		color = vec4(color0.xyz * (u_lightAmbient.xyz + diffuse), color0.w);
	}

}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program core3d vs fs
