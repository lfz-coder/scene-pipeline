#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal; // Reserved for the later lighting stage.
layout(location=2) in vec2 aUv;
layout(location=3) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uViewProjection;
out vec2 vUv;
out vec4 vColor;
void main(){vUv=aUv;vColor=aColor;gl_Position=uViewProjection*uModel*vec4(aPosition,1.0);}
