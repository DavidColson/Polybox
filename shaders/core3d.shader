@ctype mat4 Matrixf 
@ctype vec4 Vec4f 
@ctype vec3 Vec3f 

@vs vs_core3D
uniform vs_core3d_params {
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
noperspective out vec2 uv;

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
	uv = texcoord;
}
@end

@fs fs_core3D
noperspective in vec4 color;
noperspective in vec2 uv;
out vec4 frag_color;

uniform texture2D tex;
uniform sampler nearestSampler;

void main() {
	vec4 colorMixed = color * texture(sampler2D(tex, nearestSampler), uv);
	// vec3 output = mix(color.rgb, u_fogColor.rgb, v_fogDensity.x);
	frag_color = colorMixed;
}
@end

@program core3D vs_core3D fs_core3D

