@ctype mat4 Matrixf 

@vs vs
uniform vs_params {
	mat4 mvp;
};

layout(location=0) in vec4 pos;
layout(location=1) in vec4 color0;

noperspective out vec4 color;

void main() {
    gl_Position = mvp * pos;
    color = color0;
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
