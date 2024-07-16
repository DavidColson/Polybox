// Copyright 2020-2024 David Colson. All rights reserved.

@ctype mat4 Matrixf 

@vs vs_fullscreen
uniform vs_fullscreen_params {
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

@fs fs_fullscreen
in vec4 color;
in vec2 uv;
out vec4 frag_color;

uniform texture2D fullscreenFrameTex;
uniform sampler fullscreenFrameSampler;

void main()
{
	frag_color = texture(sampler2D(fullscreenFrameTex, fullscreenFrameSampler), uv);
	// frag_color = vec4(0.9, 0.9, 0.9, 1.0);
}
@end

@program fullscreen vs_fullscreen fs_fullscreen
