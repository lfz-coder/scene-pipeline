#version 330 core
in vec2 vUv;
in vec4 vColor;
out vec4 fragColor;
uniform vec4 uBaseColor;
uniform sampler2D uTexture;
uniform sampler2D uTexture2;
uniform int uUseTexture;
uniform int uUseTexture2;
uniform float uTextureMix;
uniform int uAlphaMode;
uniform float uAlphaCutoff;
uniform int uManualEncoding;
vec3 linearToSrgb(vec3 c){return mix(12.92*c,1.055*pow(max(c,vec3(0.0)),vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));}
void main(){
 vec4 color=uBaseColor*vColor;
 if(uUseTexture!=0){
  vec4 texel=texture(uTexture,vUv);
  if(uUseTexture2!=0)texel=mix(texel,texture(uTexture2,vUv),uTextureMix);
  color*=texel;
 }
 if(uAlphaMode==1 && color.a<uAlphaCutoff)discard;
 if(uAlphaMode==0)color.a=1.0;
 if(uManualEncoding!=0)color.rgb=linearToSrgb(color.rgb);
 fragColor=color;
}
