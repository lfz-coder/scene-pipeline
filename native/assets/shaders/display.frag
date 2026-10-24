#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D uLinearScene;
vec3 linearToSrgb(vec3 c){
    return mix(12.92*c,1.055*pow(max(c,vec3(0.0)),vec3(1.0/2.4))-0.055,
               step(vec3(0.0031308),c));
}
void main(){
    vec4 color=texture(uLinearScene,vUv);
    fragColor=vec4(linearToSrgb(color.rgb),color.a);
}
