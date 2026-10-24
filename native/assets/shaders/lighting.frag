#version 330 core
struct DirectionalLight { vec3 direction; vec3 color; float intensity; int enabled; };
struct PointLight { vec3 position; vec3 color; float intensity; vec3 attenuation; int enabled; };
struct SpotLight { vec3 position; vec3 direction; vec3 color; float intensity; vec3 attenuation; vec3 cone; int enabled; };
in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vUv;
in vec4 vColor;
out vec4 fragColor;
uniform DirectionalLight uDirectional;
uniform PointLight uPoints[8];
uniform SpotLight uSpots[4];
uniform int uPointCount;
uniform int uSpotCount;
uniform vec3 uViewPosition;
uniform vec4 uBaseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;
uniform vec3 uAmbientColor;
uniform float uAmbientStrength;
uniform sampler2D uTexture;
uniform sampler2D uTexture2;
uniform sampler2D uSpecularTexture;
uniform int uUseTexture;
uniform int uUseTexture2;
uniform int uUseSpecularTexture;
uniform float uTextureMix;
uniform int uAlphaMode;
uniform float uAlphaCutoff;
uniform int uManualEncoding;
uniform int uVisualizeNormals;
vec3 linearToSrgb(vec3 c){return mix(12.92*c,1.055*pow(max(c,vec3(0.0)),vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));}
vec3 directLight(vec3 lightDir,vec3 lightColor,float intensity,vec3 albedo,vec3 specular,vec3 n,vec3 viewDir){
    float diffuse=max(dot(n,lightDir),0.0);
    vec3 halfDir=normalize(lightDir+viewDir);
    float highlight=diffuse>0.0?pow(max(dot(n,halfDir),0.0),uShininess):0.0;
    return lightColor*intensity*(albedo*diffuse+specular*highlight);
}
void main(){
    vec4 base=uBaseColor*vColor;
    if(uUseTexture!=0){
        vec4 texel=texture(uTexture,vUv);
        if(uUseTexture2!=0) texel=mix(texel,texture(uTexture2,vUv),uTextureMix);
        base*=texel;
    }
    vec3 specular=uSpecularColor;
    if(uUseSpecularTexture!=0) specular*=texture(uSpecularTexture,vUv).rgb;
    vec3 n=normalize(vNormal);
    if(uAlphaMode==1 && base.a<uAlphaCutoff) discard;
    if(uAlphaMode==0) base.a=1.0;
    if(uVisualizeNormals!=0){
        vec3 normalColor=n*0.5+0.5;
        fragColor=vec4(uManualEncoding!=0?linearToSrgb(normalColor):normalColor,1.0);
        return;
    }
    vec3 viewDir=normalize(uViewPosition-vWorldPosition);
    vec3 result=uAmbientColor*uAmbientStrength*base.rgb;
    if(uDirectional.enabled!=0)
        result+=directLight(normalize(-uDirectional.direction),uDirectional.color,uDirectional.intensity,base.rgb,specular,n,viewDir);
    for(int i=0;i<uPointCount;i++){
        if(uPoints[i].enabled==0) continue;
        vec3 delta=uPoints[i].position-vWorldPosition;
        float distanceToLight=length(delta);
        vec3 lightDir=delta/max(distanceToLight,0.0001);
        float attenuation=1.0/(uPoints[i].attenuation.x+uPoints[i].attenuation.y*distanceToLight+uPoints[i].attenuation.z*distanceToLight*distanceToLight);
        result+=directLight(lightDir,uPoints[i].color,uPoints[i].intensity*attenuation,base.rgb,specular,n,viewDir);
    }
    for(int i=0;i<uSpotCount;i++){
        if(uSpots[i].enabled==0) continue;
        vec3 delta=uSpots[i].position-vWorldPosition;
        float distanceToLight=length(delta);
        vec3 lightDir=delta/max(distanceToLight,0.0001);
        float attenuation=1.0/(uSpots[i].attenuation.x+uSpots[i].attenuation.y*distanceToLight+uSpots[i].attenuation.z*distanceToLight*distanceToLight);
        float edge=dot(lightDir,normalize(-uSpots[i].direction));
        float cone=clamp((edge-uSpots[i].cone.y)/(uSpots[i].cone.x-uSpots[i].cone.y),0.0,1.0);
        result+=directLight(lightDir,uSpots[i].color,uSpots[i].intensity*attenuation*cone,base.rgb,specular,n,viewDir);
    }
    vec3 outputColor=uManualEncoding!=0?linearToSrgb(result):result;
    fragColor=vec4(outputColor,base.a);
}
