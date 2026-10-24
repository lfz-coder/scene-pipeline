#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D uSource;
uniform int uEffect;
uniform float uIntensity;
uniform int uRadius;
uniform int uHorizontal;
uniform int uManualEncoding;
uniform vec3 uTexelSize;
vec3 linearToSrgb(vec3 c){
    return mix(12.92*c,1.055*pow(max(c,vec3(0.0)),vec3(1.0/2.4))-0.055,
               step(vec3(0.0031308),c));
}
void main(){
    vec4 center=texture(uSource,vUv);
    vec3 result=center.rgb;
    if(uEffect==1) result=mix(result,vec3(1.0)-result,uIntensity);
    else if(uEffect==2){
        float gray=dot(result,vec3(0.2126,0.7152,0.0722));
        result=mix(result,vec3(gray),uIntensity);
    } else if(uEffect==3){
        vec3 neighbors=texture(uSource,vUv+vec2(uTexelSize.x,0)).rgb+
                       texture(uSource,vUv-vec2(uTexelSize.x,0)).rgb+
                       texture(uSource,vUv+vec2(0,uTexelSize.y)).rgb+
                       texture(uSource,vUv-vec2(0,uTexelSize.y)).rgb;
        result=mix(result,5.0*result-neighbors,uIntensity);
    } else if(uEffect==4){
        vec2 axis=uHorizontal!=0?vec2(uTexelSize.x,0):vec2(0,uTexelSize.y);
        float total=1.0;
        vec3 sum=result;
        for(int i=1;i<=15;i++){
            if(i>uRadius) break;
            float weight=1.0-float(i)/float(max(uRadius+1,2));
            sum+=texture(uSource,vUv+axis*float(i)).rgb*weight;
            sum+=texture(uSource,vUv-axis*float(i)).rgb*weight;
            total+=2.0*weight;
        }
        result=mix(result,sum/total,uIntensity);
    }
    if(uManualEncoding!=0) result=linearToSrgb(result);
    fragColor=vec4(result,center.a);
}
