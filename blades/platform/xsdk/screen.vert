#version 450
layout(location = 0) in vec2 a_position;
// xsdk GLSL maps location 1 to COLOR0 and location 2 to TEXCOORD0.
layout(location = 2) in vec2 a_uv;
layout(location = 0) out vec2 v_uv;

void main()
{
	gl_Position = vec4(a_position, 0.0, 1.0);
	v_uv = a_uv;
}
