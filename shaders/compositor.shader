// Copyright 2020-2024 David Colson. All rights reserved.

@ctype mat4 Matrixf 

@vs vs_compositor
uniform vs_compositor_params {
	mat4 mvp;
};

layout(location=0) in vec3 pos;
layout(location=1) in vec4 color0;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;

out vec4 color;
out vec2 uv;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
	uv = texcoord;
}
@end

@fs fs_compositor
in vec4 color;
in vec2 uv;
out vec4 frag_color;

uniform texture2D core2DFrame;
uniform texture2D core3DFrame;
uniform sampler nearestSampler;

void main()
{
	vec4 core2Dtexel = texture(sampler2D(core2DFrame, nearestSampler), uv);
	vec4 core3Dtexel = texture(sampler2D(core3DFrame, nearestSampler), uv);
	frag_color = mix(core3Dtexel, core2Dtexel, core2Dtexel.a);
}
@end

@program compositor vs_compositor fs_compositor
