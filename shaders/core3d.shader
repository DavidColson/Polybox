@ctype mat4 Matrixf 
@ctype vec4 Vec4f 
@ctype vec3 Vec3f 
@ctype vec2 Vec2f 

@vs vs_core3D
uniform vs_core3d_params {
	mat4 mvp;
	mat4 model;
	mat4 modelView;
	int lightingEnabled;
	vec4 lightDirection[3];
	vec4 lightColor[3];
	vec3 lightAmbient;
	int fogEnabled;
	vec2 fogDepths;
	vec2 targetResolution;
};

layout(location=0) in vec3 pos;
layout(location=1) in vec4 color0;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;

noperspective out vec4 color;
noperspective out vec2 uv;
out float fogDensity;

void main() {
	vec2 resolution = targetResolution.xy * 0.5;
    vec4 vert = mvp * vec4(pos, 1.0);

	// Snap vertices to screen pixels
	vec4 snapped = vert;
	snapped.xyz = vert.xyz / vert.w;
	snapped.xy = floor(resolution * snapped.xy) / resolution;
	snapped.xyz *= vert.w;

	gl_Position = snapped;

	if (fogEnabled == 1)
	{
		vec4 depthVert = modelView * vec4(pos, 1.0);
		float depth = abs(depthVert.z / depthVert.w);
		fogDensity = 1.0 - clamp((fogDepths.y - depth) / (fogDepths.y - fogDepths.x), 0.0, 1.0);
	}

	if (lightingEnabled == 0)
	{
		color = color0;
	}
	else if (lightingEnabled == 1)
	{
		vec3 norm = (model * vec4(normal, 0.0)).xyz;
		float lightMag = max(dot(normalize(lightDirection[0].xyz), norm.xyz), 0.0);
		vec3 diffuse = lightMag * lightColor[0].xyz;

		color = vec4(color0.xyz * (lightAmbient.xyz + diffuse), color0.w);
	}
	uv = texcoord;
}
@end

@fs fs_core3D
noperspective in vec4 color;
noperspective in vec2 uv;
in float fogDensity;
out vec4 frag_color;

uniform fs_core3d_params {
	vec4 fogColor;
};

uniform texture2D tex;
uniform sampler nearestSampler;

vec4 RGBtoYUV(vec4 rgba) {
	vec4 yuva;
	yuva.r = rgba.r * 0.2126 + 0.7152 * rgba.g + 0.0722 * rgba.b;
	yuva.g = (rgba.b - yuva.r) / 1.8556;
	yuva.b = (rgba.r - yuva.r) / 1.5748;
	yuva.a = rgba.a;
	
	// Adjust to work on GPU
	yuva.gb += 0.5;
	
	return yuva;
}

vec4 YUVtoRGB(vec4 yuva) {
	yuva.gb -= 0.5;
	return vec4(
		yuva.r * 1 + yuva.g * 0 + yuva.b * 1.5748,
		yuva.r * 1 + yuva.g * -0.187324 + yuva.b * -0.468124,
		yuva.r * 1 + yuva.g * 1.8556 + yuva.b * 0,
		yuva.a);
}

float dither8x8(vec2 position, float brightness)
{
	int x = int(mod(position.x, 8.0));
  	int y = int(mod(position.y, 8.0));

	int dither[8][8] = {
	{ 0, 32, 8, 40, 2, 34, 10, 42}, /* 8x8 Bayer ordered dithering */
	{48, 16, 56, 24, 50, 18, 58, 26}, /* pattern. Each input pixel */
	{12, 44, 4, 36, 14, 46, 6, 38}, /* is scaled to the 0..63 range */
	{60, 28, 52, 20, 62, 30, 54, 22}, /* before looking in this table */
	{ 3, 35, 11, 43, 1, 33, 9, 41}, /* to determine th  e action. */
	{51, 19, 59, 27, 49, 17, 57, 25},
	{15, 47, 7, 39, 13, 45, 5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21} }; 

	float limit = 0.0;
	if(x < 8)
		limit = (dither[x][y]+1)/64.0;

	return brightness < limit ? 0.0 : 1.0;
}

float ditherChannelError(float col, float colMin, float colMax)
{
	float range = abs(colMin - colMax);
	float aRange = abs(col - colMin);
	return aRange /range;
}

vec4 ditherAndPosterize(vec2 position, vec4 color, int colorDepth)
{
	vec4 yuv = RGBtoYUV(color);

	vec4 col1 = floor(yuv * colorDepth) / colorDepth;
	vec4 col2 = ceil(yuv * colorDepth) / colorDepth;

	yuv.x = mix(col1.x, col2.x, dither8x8(position, ditherChannelError(yuv.x, col1.x, col2.x)));
	yuv.y = mix(col1.y, col2.y, dither8x8(position, ditherChannelError(yuv.y, col1.y, col2.y)));
	yuv.z = mix(col1.z, col2.z, dither8x8(position, ditherChannelError(yuv.z, col1.z, col2.z)));

	return YUVtoRGB(yuv);
}

void main() {
	vec4 colorTextured = color * texture(sampler2D(tex, nearestSampler), uv);
	vec3 fog_output = mix(colorTextured.rgb, fogColor.rgb, fogDensity);
	frag_color = vec4(fog_output, colorTextured.a);

	// 32 color depth means 32 values are possible per component
	// which is 2^5, so it's a 15 bit pixel format
	frag_color = ditherAndPosterize(gl_FragCoord.xy, frag_color, 32);
}
@end

@program core3D vs_core3D fs_core3D

