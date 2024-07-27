@ctype mat4 Matrixf 
@ctype vec4 Vec4f 
@ctype vec3 Vec3f 

@vs vs_core2D
uniform vs_core2d_params {
	mat4 mvp;
	mat4 model;
};

layout(location=0) in vec3 pos;
layout(location=1) in vec4 color0;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;

out vec4 color;
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(pos, 1.0);
    color = color0;
	uv = texcoord;
}
@end

@fs fs_core2DNonTextured
in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@fs fs_core2DTextured
in vec4 color;
in vec2 uv;
out vec4 frag_color;

uniform texture2D tex;
uniform sampler nearestSampler;

void main() {
	vec4 colorMixed = color * texture(sampler2D(tex, nearestSampler), uv);
	frag_color = colorMixed;
}
@end

@program core2DTextured vs_core2D fs_core2DTextured
@program core2DNonTextured vs_core2D fs_core2DNonTextured

