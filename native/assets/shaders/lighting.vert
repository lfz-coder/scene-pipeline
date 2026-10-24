#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUv;
layout(location=3) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vUv;
out vec4 vColor;
void main(){
    vec4 world=uModel*vec4(aPosition,1.0);
    vWorldPosition=world.xyz;
    vNormal=normalize(uNormalMatrix*aNormal);
    vUv=aUv;
    vColor=aColor;
    gl_Position=uProjection*uView*world;
}
