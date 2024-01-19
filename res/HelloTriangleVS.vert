#version 450

// pain
/*
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
vec3(1.0, 0.0, 0.0),
vec3(0.0, 1.0, 0.0),
vec3(0.0, 0.0, 1.0)
);
*/

layout(location = 0) in vec2 vPositiion;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec3 fColor;

void main() {
    gl_Position = vec4(vPositiion.xy, 0.0, 1.0);//= vec4(vPositiion/*positions[gl_VertexIndex]*/, 0.0, 1.0);
    fColor      = vColor;//= vColor;/*colors[gl_VertexIndex]*/;
}