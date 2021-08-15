$input a_position, a_texcoord0, a_color0, a_normal
$output v_color0, v_texcoord0

#include "common.sh"

uniform vec4 u_targetResolution;
uniform vec4 u_lightMode;
uniform vec4 u_lightDirection[3];
uniform vec4 u_lightColor[3];
uniform vec4 u_lightAmbient;

void main()
{
	vec2 resolution = u_targetResolution.xy;
	vec4 vert = mul(u_modelViewProj, vec4(a_position, 1.0) );
	vec3 norm = mul(u_model[0], vec4(a_normal.xyz, 0.0) ).xyz;

	// Snap vertices to screen pixels
	vec4 snapped = vert;
	snapped.xyz = vert.xyz / vert.w;
	snapped.xy = floor(resolution * snapped.xy) / resolution;
	snapped.xyz *= vert.w;

	gl_Position = snapped;

	int lightMode = int(u_lightMode.x);
	switch(lightMode)
	{
	case 0:
		v_color0 = a_color0;
		break;
	case 1:
		float lightMag = max(dot(normalize(u_lightDirection[0].xyz), norm.xyz), 0.0);
		vec3 diffuse = lightMag * u_lightColor[0];

		v_color0 = vec4(a_color0.xyz * (u_lightAmbient.xyz + diffuse), a_color0.z);
		break;
	case 2:
		v_color0 = a_color0;
		break;
	}
	v_texcoord0 = a_texcoord0;
}
