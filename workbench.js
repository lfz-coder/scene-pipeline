/* LUMA / 光场工坊 — independent, dependency-free WebGL2 product prototype.
 * Pipeline: scene -> shadow map -> material/toon/outline -> color FBO -> bloom -> display.
 * Open index.html directly: no server, build tool or network asset is required.
 */
(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
  const V = {
    add:(a,b)=>a.map((x,i)=>x+b[i]), sub:(a,b)=>a.map((x,i)=>x-b[i]),
    mul:(a,s)=>a.map(x=>x*s), dot:(a,b)=>a.reduce((s,x,i)=>s+x*b[i],0),
    cross:(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]],
    norm:a=>{const l=Math.hypot(...a)||1;return a.map(x=>x/l);}
  };
  const Mat4 = {
    identity(){return new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]);},
    multiply(a,b){const o=new Float32Array(16);for(let c=0;c<4;c++)for(let r=0;r<4;r++)o[c*4+r]=a[r]*b[c*4]+a[4+r]*b[c*4+1]+a[8+r]*b[c*4+2]+a[12+r]*b[c*4+3];return o;},
    perspective(fov,aspect,near,far){const f=1/Math.tan(fov/2),nf=1/(near-far);return new Float32Array([f/aspect,0,0,0,0,f,0,0,0,0,(far+near)*nf,-1,0,0,2*near*far*nf,0]);},
    ortho(l,r,b,t,n,f){return new Float32Array([2/(r-l),0,0,0,0,2/(t-b),0,0,0,0,-2/(f-n),0,-(r+l)/(r-l),-(t+b)/(t-b),-(f+n)/(f-n),1]);},
    lookAt(eye,at,up){const z=V.norm(V.sub(eye,at)),x=V.norm(V.cross(up,z)),y=V.cross(z,x);return new Float32Array([x[0],y[0],z[0],0,x[1],y[1],z[1],0,x[2],y[2],z[2],0,-V.dot(x,eye),-V.dot(y,eye),-V.dot(z,eye),1]);},
    model(p,rotation=[0,0,0],scale=[1,1,1]){
      const [x,y,z]=rotation,cx=Math.cos(x),sx=Math.sin(x),cy=Math.cos(y),sy=Math.sin(y),cz=Math.cos(z),sz=Math.sin(z);
      const rx=new Float32Array([1,0,0,0,0,cx,sx,0,0,-sx,cx,0,0,0,0,1]);
      const ry=new Float32Array([cy,0,-sy,0,0,1,0,0,sy,0,cy,0,0,0,0,1]);
      const rz=new Float32Array([cz,sz,0,0,-sz,cz,0,0,0,0,1,0,0,0,0,1]);
      const o=this.multiply(this.multiply(ry,rx),rz);for(let c=0;c<3;c++)for(let r=0;r<3;r++)o[c*4+r]*=scale[c];o[12]=p[0];o[13]=p[1];o[14]=p[2];return o;
    }
  };
  class Camera {
    constructor(){this.reset();}
    reset(scene='courier'){this.target=scene==='garden'?[0,.55,0]:scene==='materials'?[0,.45,0]:[0,1.15,0];this.yaw=.39;this.pitch=.23;this.distance=scene==='materials'?10.5:scene==='garden'?9.6:9.2;this.position=[0,0,0];}
    view(){const cp=Math.cos(this.pitch);this.position=V.add(this.target,[Math.sin(this.yaw)*cp*this.distance,Math.sin(this.pitch)*this.distance,Math.cos(this.yaw)*cp*this.distance]);return Mat4.lookAt(this.position,this.target,[0,1,0]);}
    projection(aspect){return Mat4.perspective(43*Math.PI/180,aspect,.08,80);}
    orbit(dx,dy){this.yaw-=dx*.006;this.pitch=clamp(this.pitch+dy*.005,-.08,1.35);}
    pan(dx,dy){const right=[Math.cos(this.yaw),0,-Math.sin(this.yaw)];this.target=V.add(this.target,V.mul(right,-dx*this.distance*.0013));this.target[1]+=dy*this.distance*.0013;}
    update(dt,keys){const f=[-Math.sin(this.yaw),0,-Math.cos(this.yaw)],r=[Math.cos(this.yaw),0,-Math.sin(this.yaw)];let move=[0,0,0];if(keys.has('KeyW'))move=V.add(move,f);if(keys.has('KeyS'))move=V.sub(move,f);if(keys.has('KeyD'))move=V.add(move,r);if(keys.has('KeyA'))move=V.sub(move,r);if(keys.has('KeyE'))move[1]+=1;if(keys.has('KeyQ'))move[1]-=1;if(V.dot(move,move)>0)this.target=V.add(this.target,V.mul(V.norm(move),dt*3));}
  }
  class Mesh {
    constructor(gl,vertices,indices){
      this.gl=gl;this.count=indices.length;this.triangles=indices.length/3;this.vao=gl.createVertexArray();this.buffers=[];
      gl.bindVertexArray(this.vao);const vbo=gl.createBuffer();this.buffers.push(vbo);gl.bindBuffer(gl.ARRAY_BUFFER,vbo);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(vertices),gl.STATIC_DRAW);
      this.ebo=gl.createBuffer();this.buffers.push(this.ebo);gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,this.ebo);gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,new Uint32Array(indices),gl.STATIC_DRAW);
      for(let i=0;i<3;i++){gl.enableVertexAttribArray(i);gl.vertexAttribPointer(i,i===2?4:3,gl.FLOAT,false,40,[0,12,24][i]);}
      const edges=new Set(),lines=[];for(let i=0;i<indices.length;i+=3)for(const [a,b]of[[indices[i],indices[i+1]],[indices[i+1],indices[i+2]],[indices[i+2],indices[i]]]){const k=a<b?`${a}:${b}`:`${b}:${a}`;if(!edges.has(k)){edges.add(k);lines.push(a,b);}}
      this.lineEbo=gl.createBuffer();this.buffers.push(this.lineEbo);this.lineCount=lines.length;gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,this.lineEbo);gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,new Uint32Array(lines),gl.STATIC_DRAW);gl.bindVertexArray(null);
    }
    draw(lines=false){const g=this.gl;g.bindVertexArray(this.vao);g.bindBuffer(g.ELEMENT_ARRAY_BUFFER,lines?this.lineEbo:this.ebo);g.drawElements(lines?g.LINES:g.TRIANGLES,lines?this.lineCount:this.count,g.UNSIGNED_INT,0);}
  }
  const MeshFactory = {
    cube(gl){
      const faces=[[[0,0,1],[[-1,-1,1],[1,-1,1],[1,1,1],[-1,1,1]]],[[0,0,-1],[[1,-1,-1],[-1,-1,-1],[-1,1,-1],[1,1,-1]]],[[0,1,0],[[-1,1,1],[1,1,1],[1,1,-1],[-1,1,-1]]],[[0,-1,0],[[-1,-1,-1],[1,-1,-1],[1,-1,1],[-1,-1,1]]],[[1,0,0],[[1,-1,1],[1,-1,-1],[1,1,-1],[1,1,1]]],[[-1,0,0],[[-1,-1,-1],[-1,-1,1],[-1,1,1],[-1,1,-1]]]];
      const v=[],ix=[];faces.forEach(([n,ps],i)=>{ps.forEach(p=>v.push(...p,...n,1,1,1,1));const b=i*4;ix.push(b,b+1,b+2,b,b+2,b+3);});return new Mesh(gl,v,ix);
    },
    plane(gl){return new Mesh(gl,[-1,0,-1,0,1,0,1,1,1,1,1,0,-1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,-1,0,1,0,1,0,1,1,1,1],[0,2,1,0,3,2]);},
    sphere(gl,segments=32,rings=20){
      const v=[],ix=[];for(let y=0;y<=rings;y++){const phi=y/rings*Math.PI;for(let x=0;x<=segments;x++){const t=x/segments*Math.PI*2,p=[Math.sin(phi)*Math.cos(t),Math.cos(phi),Math.sin(phi)*Math.sin(t)];v.push(...p,...p,1,1,1,1);}}
      for(let y=0;y<rings;y++)for(let x=0;x<segments;x++){const a=y*(segments+1)+x,b=a+segments+1;ix.push(a,a+1,b,b,a+1,b+1);}return new Mesh(gl,v,ix);
    },
    cylinder(gl,segments=40){
      const v=[],ix=[];for(let y=0;y<=1;y++)for(let x=0;x<=segments;x++){const t=x/segments*Math.PI*2,n=[Math.cos(t),0,Math.sin(t)];v.push(n[0],y*2-1,n[2],...n,1,1,1,1);}
      for(let x=0;x<segments;x++){const a=x,b=x+segments+1;ix.push(a,b,a+1,b,b+1,a+1);}
      for(const sign of[-1,1]){const center=v.length/10;v.push(0,sign,0,0,sign,0,1,1,1,1);for(let x=0;x<=segments;x++){const t=x/segments*Math.PI*2;v.push(Math.cos(t),sign,Math.sin(t),0,sign,0,1,1,1,1);}for(let x=0;x<segments;x++)if(sign===1)ix.push(center,center+x+2,center+x+1);else ix.push(center,center+x+1,center+x+2);}
      return new Mesh(gl,v,ix);
    },
    torus(gl,segments=48,rings=12){
      const v=[],ix=[],tube=.09;for(let y=0;y<=rings;y++){const p=y/rings*Math.PI*2;for(let x=0;x<=segments;x++){const t=x/segments*Math.PI*2,n=[Math.cos(p)*Math.cos(t),Math.sin(p),Math.cos(p)*Math.sin(t)];v.push((1+tube*Math.cos(p))*Math.cos(t),tube*Math.sin(p),(1+tube*Math.cos(p))*Math.sin(t),...n,1,1,1,1);}}
      for(let y=0;y<rings;y++)for(let x=0;x<segments;x++){const a=y*(segments+1)+x,b=a+segments+1;ix.push(a,b,a+1,b,b+1,a+1);}return new Mesh(gl,v,ix);
    }
  };
  class ShaderProgram {
    constructor(gl,vs,fs){this.gl=gl;this.locations=new Map();const compile=(type,src)=>{const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)){const msg=gl.getShaderInfoLog(s);gl.deleteShader(s);throw new Error(msg);}return s;};const v=compile(gl.VERTEX_SHADER,vs),f=compile(gl.FRAGMENT_SHADER,fs);this.program=gl.createProgram();gl.attachShader(this.program,v);gl.attachShader(this.program,f);gl.linkProgram(this.program);gl.deleteShader(v);gl.deleteShader(f);if(!gl.getProgramParameter(this.program,gl.LINK_STATUS))throw new Error(gl.getProgramInfoLog(this.program));}
    use(){this.gl.useProgram(this.program);return this;}
    loc(n){if(!this.locations.has(n))this.locations.set(n,this.gl.getUniformLocation(this.program,n));return this.locations.get(n);}
    mat4(n,v){this.gl.uniformMatrix4fv(this.loc(n),false,v);return this;}
    vec3(n,v){this.gl.uniform3fv(this.loc(n),v);return this;}
    float(n,v){this.gl.uniform1f(this.loc(n),v);return this;}
    int(n,v){this.gl.uniform1i(this.loc(n),v);return this;}
    vec2(n,v){this.gl.uniform2fv(this.loc(n),v);return this;}
  }
  const surfaceVertex=`#version 300 es
  layout(location=0) in vec3 aPosition; layout(location=1) in vec3 aNormal;
  uniform mat4 uModel,uView,uProjection,uLightVP; uniform float uExpand;
  out vec3 vWorldPos,vNormal; out vec4 vShadow;
  void main(){vec3 pos=aPosition*(1.+uExpand);vec4 p=uModel*vec4(pos,1.);vWorldPos=p.xyz;vNormal=transpose(inverse(mat3(uModel)))*aNormal;vShadow=uLightVP*p;gl_Position=uProjection*uView*p;}`;
  function surfaceFragment(mode){return `#version 300 es
  precision highp float;
  in vec3 vWorldPos,vNormal; in vec4 vShadow;
  uniform vec3 uCamera,uLight,uColor; uniform float uMetallic,uRoughness,uExposure,uEmission,uShadow,uGrid,uGround,uEnvironment,uTime;
  uniform sampler2D uShadowMap; out vec4 outColor;
  const float PI=3.14159265; const int STYLE=${mode};
  float shadow(vec3 n,vec3 l){if(uShadow<.5)return 1.;vec3 p=vShadow.xyz/vShadow.w*.5+.5;if(p.z>1.||p.z<0.||p.x<0.||p.x>1.||p.y<0.||p.y>1.)return 1.;float bias=max(.002*(1.-dot(n,l)),.0005),s=0.;vec2 texel=1./vec2(textureSize(uShadowMap,0));for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){float d=texture(uShadowMap,p.xy+vec2(x,y)*texel).r;s+=p.z-bias<=d?1.:.15;}return s/9.;}
  vec3 environment(vec3 r,float rough){vec3 e=mix(vec3(.08,.1,.16),vec3(.55,.62,.72),r.y*.5+.5);e+=vec3(2.5,2.3,2.1)*pow(max(dot(r,normalize(vec3(-.45,.7,1.))),0.),mix(150.,4.,rough));e+=vec3(.4,.6,1.2)*pow(max(dot(r,normalize(vec3(1.,.3,-.4))),0.),mix(90.,3.,rough));return e;}
  vec3 tonemap(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.);}
  void main(){vec3 n=normalize(vNormal),v=normalize(uCamera-vWorldPos),l=normalize(uLight),h=normalize(v+l);float nv=max(dot(n,v),.001),nl=max(dot(n,l),0.),nh=max(dot(n,h),0.),vh=max(dot(v,h),0.);float sh=shadow(n,l);vec3 base=pow(uColor,vec3(2.2)),c;
    if(STYLE==0){float r=max(uRoughness,.06),a=r*r,a2=a*a;float denom=nh*nh*(a2-1.)+1.;float d=a2/(PI*denom*denom+.00001);float k=(r+1.)*(r+1.)/8.;float g=(nv/(nv*(1.-k)+k))*(nl/(nl*(1.-k)+k));vec3 f0=mix(vec3(.04),base,uMetallic),f=f0+(1.-f0)*pow(1.-vh,5.);vec3 spec=d*g*f/(4.*nv*max(nl,.001)+.0001);vec3 diffuse=(1.-f)*(1.-uMetallic)*base/PI;vec3 direct=(diffuse+spec)*vec3(3.8,3.35,2.9)*nl*sh;vec3 refl=reflect(-v,n);vec3 ef=f0+(1.-f0)*pow(1.-nv,5.);vec3 ambient=base*(1.-uMetallic)*mix(vec3(.1,.12,.18),vec3(.33,.39,.48),n.y*.5+.5);c=direct+ambient+environment(refl,r)*ef*(1.-r*.6);c+=base*uEmission*2.;c=pow(tonemap(c*uExposure),vec3(1./2.2));
    }else if(STYLE==1){float lambert=dot(n,l)*.5+.5;float band=lambert>.78?1.:lambert>.5?.8:lambert>.25?.6:.43;band*=mix(.65,1.,sh);vec3 shade=mix(vec3(.64,.49,.72),vec3(1.03,1.,1.02),band);float spec=step(.975,nh)*uMetallic*.24;float rim=pow(1.-nv,4.)*.08;c=uColor*shade+spec+rim;c+=uColor*uEmission*.45;c*=pow(uExposure,.5);
    }else if(STYLE==2){float edge=pow(1.-nv,2.1);float stripe=pow(max(0.,sin(vWorldPos.y*10.+uTime*.6)),18.);vec3 glow=mix(vec3(.08,.78,1.),vec3(.85,.25,.9),clamp(vWorldPos.y/3.5,0.,1.));c=uColor*.07+glow*edge*1.1+vec3(.02,.04,.08)*nl;c+=mix(vec3(.15,.8,1.),vec3(1.,.15,.7),sin(vWorldPos.y*.7)*.5+.5)*edge*.5;c+=uColor*uEmission*1.4+uColor*stripe*.05;c*=uExposure;
    }else{c=vec3(.035,.065,.09);}
    if(STYLE==2&&uEnvironment>.5)c*=.22;if(STYLE==2&&uGround>.5)c=vec3(.025,.03,.055);
    if(uGrid>.5){vec2 coord=vWorldPos.xz;vec2 grid=abs(fract(coord-.5)-.5)/max(fwidth(coord),vec2(.001));float line=1.-min(min(grid.x,grid.y),1.);c=mix(c,STYLE==1?vec3(.58,.51,.67):vec3(.35,.46,.53),line*.24);}
    if(uGround>.5){vec3 horizon=STYLE==1?vec3(.85,.84,.91):STYLE==2?vec3(.045,.03,.095):vec3(.185,.20,.24);c=mix(c,horizon,smoothstep(7.,25.,length(vWorldPos.xz)));}outColor=vec4(c,1.);
  }`;}
  const outlineFragment=`#version 300 es
  precision highp float; uniform vec3 uOutline;out vec4 outColor;void main(){outColor=vec4(uOutline,1.);}`;
  const depthVertex=`#version 300 es
  layout(location=0)in vec3 aPosition;uniform mat4 uModel,uLightVP;void main(){gl_Position=uLightVP*uModel*vec4(aPosition,1.);}`;
  const depthFragment=`#version 300 es
  precision highp float;void main(){}`;
  const pickFragment=`#version 300 es
  precision highp float;uniform float uId;out vec4 outColor;void main(){outColor=vec4(uId/255.,0.,0.,1.);}`;
  const quadVertex=`#version 300 es
  out vec2 vUv;void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));vUv=p;gl_Position=vec4(p*2.-1.,0.,1.);}`;
  const backgroundFragment=`#version 300 es
  precision highp float;in vec2 vUv;uniform int uStyle;out vec4 outColor;void main(){vec3 top,bottom;if(uStyle==1){top=vec3(.89,.86,.93);bottom=vec3(.77,.8,.87);}else if(uStyle==2){top=vec3(.034,.025,.08);bottom=vec3(.065,.04,.13);}else if(uStyle==3){top=vec3(.035,.065,.09);bottom=vec3(.06,.10,.13);}else{top=vec3(.14,.16,.21);bottom=vec3(.26,.27,.29);}vec3 c=mix(bottom,top,vUv.y);float light=exp(-length((vUv-vec2(.57,.64))*vec2(1.,1.3))*3.);c+=light*(uStyle==1?.045:.035);float grain=fract(sin(dot(gl_FragCoord.xy,vec2(12.9898,78.233)))*43758.5453);c+=(grain-.5)*.004;outColor=vec4(c,1.);}`;
  const blurFragment=`#version 300 es
  precision highp float;in vec2 vUv;uniform sampler2D uSource;uniform vec2 uDirection;uniform float uExtract;out vec4 outColor;
  vec3 sampleColor(vec2 p){vec3 c=texture(uSource,p).rgb;return uExtract>.5?c*smoothstep(.56,.98,max(max(c.r,c.g),c.b)):c;}
  void main(){vec3 c=sampleColor(vUv)*.227027;c+=sampleColor(vUv+uDirection*1.384615)*.316216;c+=sampleColor(vUv-uDirection*1.384615)*.316216;c+=sampleColor(vUv+uDirection*3.230769)*.070270;c+=sampleColor(vUv-uDirection*3.230769)*.070270;outColor=vec4(c,1.);}`;
  const compositeFragment=`#version 300 es
  precision highp float;in vec2 vUv;uniform sampler2D uSource,uBloom;uniform float uBloomStrength;out vec4 outColor;void main(){vec3 c=texture(uSource,vUv).rgb;c+=texture(uBloom,vUv).rgb*uBloomStrength;float vignette=1.-dot(vUv-.5,vUv-.5)*.16;c*=vignette;outColor=vec4(c,1.);}`;

  const styleInfo={realistic:{id:0,name:'写实 · Studio',tag:'REALISTIC STUDIO'},toon:{id:1,name:'二次元 · Toon',tag:'ANIME TOON'},neon:{id:2,name:'霓虹 · Neon',tag:'NEON GLOW'},wireframe:{id:3,name:'线框 · Wireframe',tag:'WIREFRAME'}};
  const sceneInfo={courier:{title:'星际信使 A–01',subtitle:'CHARACTER LOOK DEVELOPMENT',description:'为同一个角色，找到不同世界的模样。'},materials:{title:'光与材质的五种相遇',subtitle:'MATERIAL EXPLORATION',description:'从陶瓷到金属，探索表面如何回应光。'},garden:{title:'天空庭院 · 午后',subtitle:'ENVIRONMENT LOOK DEVELOPMENT',description:'一座小屋、一树春色，和属于你的光影。'}};
  const hexRGB=hex=>[1,3,5].map(i=>parseInt(hex.slice(i,i+2),16)/255);
  const mat=(color,metal=.15,rough=.45,emission=0,theme=false)=>({color,metal,rough,emission,theme});
  class Entity {
    constructor(id,name,color,metal=.35,rough=.32,type='PROCEDURAL MESH'){this.id=id;this.name=name;this.color=color;this.metal=metal;this.rough=rough;this.type=type;this.parts=[];this.position=[0,0,0];this.rotation=0;this.scale=1;}
    add(mesh,p,s,material,rotation=[0,0,0]){this.parts.push({mesh,p,s,material,rotation});return this;}
    matrix(){return Mat4.model(this.position,[0,this.rotation*Math.PI/180,0],[this.scale,this.scale,this.scale]);}
  }
  class Scene {
    constructor(meshes){this.meshes=meshes;this.entities=[];this.preset='courier';this.create('courier');}
    create(preset){
      this.preset=preset;this.entities=[];const m=this.meshes;const add=(name,color,metal,rough,type)=>{const e=new Entity(this.entities.length+1,name,color,metal,rough,type);this.entities.push(e);return e;};
      const theme=mat('#b6a5e9',.35,.32,0,true),white=mat('#e8e6e2',.2,.25),dark=mat('#242a3a',.5,.24),mint=mat('#78e4d6',.15,.25,.6),gold=mat('#deb680',.8,.28);
      const env=new Entity(0,'场景环境','#999999');this.environment=env;
      env.add(m.plane,[0,-.66,0],[60,1,60],mat('#777c87',.12,.8));
      if(preset==='courier'){
        env.add(m.cylinder,[0,-.42,0],[2.4,.21,2.4],mat('#a4a5b5',.4,.36));
        env.add(m.torus,[0,-.19,0],[2.18,.28,2.18],mat('#c1afff',.3,.3,.12));
        const bot=add('信使 A–01','#b6a5e9',.35,.32,'PROCEDURAL CHARACTER');bot.position[1]=-.26;
        bot.add(m.sphere,[0,1.43,0],[.68,.8,.52],theme).add(m.sphere,[0,2.6,0],[.96,.78,.72],white);
        bot.add(m.sphere,[0,2.59,.49],[.77,.48,.39],dark);
        for(const x of[-1,1]){
          bot.add(m.sphere,[x*.31,2.62,.854],[.105,.145,.055],mint);
          bot.add(m.sphere,[x*.93,2.58,-.04],[.19,.36,.37],theme);
          bot.add(m.cylinder,[x*.78,1.53,0],[.2,.44,.2],white,[0,0,x*.23]);
          bot.add(m.sphere,[x*.88,1.08,.03],[.24,.27,.25],theme);
          bot.add(m.cylinder,[x*.31,.62,0],[.18,.31,.18],dark);
          bot.add(m.sphere,[x*.32,.25,.13],[.29,.2,.44],white);
          bot.add(m.sphere,[x*.32,.25,.47],[.16,.08,.04],mint);
        }
        bot.add(m.sphere,[0,1.57,.45],[.27,.31,.09],white).add(m.sphere,[0,1.6,.535],[.11,.11,.035],mint);
        bot.add(m.cylinder,[.46,3.38,0],[.035,.3,.035],gold,[0,0,-.2]).add(m.sphere,[.51,3.69,0],[.12,.12,.12],mint);
        bot.add(m.cube,[0,1.6,-.52],[.42,.5,.16],dark);
        const satellite=add('轨道伙伴','#e8b8ad',.65,.2,'ORBITAL COMPANION');
        satellite.add(m.sphere,[-2.25,1.2,-.45],[.38,.38,.38],theme).add(m.torus,[-2.25,1.2,-.45],[.63,.63,.63],gold,[.3,0,.27]);
        satellite.add(m.sphere,[-2.25,1.2,-.13],[.14,.1,.065],mint);
        const beacon=add('信号灯塔','#84c8b8',.5,.22,'DISPLAY OBJECT');
        beacon.add(m.cylinder,[2.0,.03,.1],[.43,.32,.43],dark).add(m.sphere,[2, .57,.1],[.38,.38,.38],theme);
        beacon.add(m.torus,[2,.43,.1],[.47,.4,.47],mint).add(m.torus,[2,.77,.1],[.3,.3,.3],mint);
      }else if(preset==='materials'){
        const configs=[['香槟金属','#d8b577',1,.19],['樱粉陶瓷','#d99aad',.05,.25],['月光铬银','#c7d1da',1,.08],['薄荷磨砂','#8fc7b1',.08,.78],['紫晶涂层','#9b91cf',.6,.27]];
        configs.forEach(([name,color,metal,rough],i)=>{const x=(i-2)*1.65,z=Math.abs(i-2)*-.35,height=i===2?.45:.05;
          env.add(m.cylinder,[x,-.38+height/2,z],[.72,.27+height/2,.72],mat('#6c707d',.3,.48));
          const sphere=add(name,color,metal,rough,'MATERIAL SAMPLE');sphere.add(m.sphere,[x,.61+height,z],[.68,.68,.68],theme);
        });
      }else{
        env.add(m.cube,[0,-.42,0],[2.8,.22,2.45],mat('#969db0',.12,.7));env.add(m.cube,[0,-.18,0],[2.75,.04,2.4],mat('#8fba9b',.05,.9));
        const house=add('庭院小屋','#d7b7a9',.1,.62,'MODULAR ARCHITECTURE');
        house.add(m.cube,[.5,.66,-.55],[.97,.84,.88],theme).add(m.cube,[.5,1.56,-.55],[1.12,.1,1.03],white);
        house.add(m.cube,[.5,1.8,-.6],[.68,.16,.64],mat('#8d8b97',.3,.5));
        for(const x of[-.09,.94])house.add(m.cube,[x,.7,.343],[.38,.6,.025],mat('#4e6e81',.65,.18));
        for(const x of[-.48,.45,1.44])house.add(m.cube,[x,.68,.385],[.035,.77,.045],gold);
        house.add(m.cube,[.5,.72,.39],[.98,.024,.04],gold);
        house.add(m.cube,[.5,-.05,.75],[1.25,.1,.45],white);
        for(let i=0;i<4;i++)env.add(m.cube,[-.35+i*.28,-.08,1.4+i*.12],[.24,.035,.18],mat('#d5d1c7',.1,.7));
        const tree=add('一树春色','#e9b0c5',.05,.85,'STYLIZED FOLIAGE');
        tree.add(m.cylinder,[-1.65,.65,-.65],[.1,.84,.1],mat('#806d70',0,.9),[0,0,-.13]);
        for(const [x,y,z,s]of[[-1.65,1.75,-.65,.8],[-2.1,1.4,-.35,.6],[-1.3,1.45,-.12,.65],[-1.4,2.08,-.7,.55],[-1.94,1.9,-1,.58]])tree.add(m.sphere,[x,y,z],[s,s*.72,s],theme);
        const garden=add('庭院陈设','#a5c8b2',.15,.5,'GARDEN PROPS');
        for(const x of[-1.8,1.9]){garden.add(m.cylinder,[x,0,1.05],[.23,.22,.23],mat('#d6bba3',.1,.7));garden.add(m.sphere,[x,.35,1.05],[.33,.4,.33],theme);}
        garden.add(m.cube,[1.85,.3,-1.3],[.38,.06,.24],gold);for(const x of[1.55,2.15])garden.add(m.cube,[x,.05,-1.3],[.04,.24,.2],dark);
      }
      this.selectedId=1;
    }
    get selected(){return this.entities.find(e=>e.id===this.selectedId)||this.entities[0];}
    objects(){const items=[];for(const e of[this.environment,...this.entities]){const group=e.matrix();for(const part of e.parts)items.push({entity:e,part,model:Mat4.multiply(group,Mat4.model(part.p,part.rotation,part.s))});}return items;}
  }
  class RenderTarget {
    constructor(gl,w,h,depth=false){this.gl=gl;this.w=w;this.h=h;this.fbo=gl.createFramebuffer();this.texture=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,this.texture);gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA8,w,h,0,gl.RGBA,gl.UNSIGNED_BYTE,null);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);gl.bindFramebuffer(gl.FRAMEBUFFER,this.fbo);gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,gl.TEXTURE_2D,this.texture,0);if(depth){this.depth=gl.createRenderbuffer();gl.bindRenderbuffer(gl.RENDERBUFFER,this.depth);gl.renderbufferStorage(gl.RENDERBUFFER,gl.DEPTH_COMPONENT24,w,h);gl.framebufferRenderbuffer(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.RENDERBUFFER,this.depth);}if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE)throw new Error('颜色帧缓冲创建失败');gl.bindFramebuffer(gl.FRAMEBUFFER,null);}
    bind(){this.gl.bindFramebuffer(this.gl.FRAMEBUFFER,this.fbo);this.gl.viewport(0,0,this.w,this.h);}
    dispose(){this.gl.deleteFramebuffer(this.fbo);this.gl.deleteTexture(this.texture);if(this.depth)this.gl.deleteRenderbuffer(this.depth);}
  }
  class StylePipeline {
    constructor(gl){this.styles={};for(const [key,info]of Object.entries(styleInfo))this.styles[key]=new ShaderProgram(gl,surfaceVertex,surfaceFragment(info.id));this.outline=new ShaderProgram(gl,surfaceVertex,outlineFragment);}
    get(name){return this.styles[name];}
  }
  class Renderer {
    constructor(gl,canvas){
      this.gl=gl;this.canvas=canvas;this.pipeline=new StylePipeline(gl);this.depthProgram=new ShaderProgram(gl,depthVertex,depthFragment);this.pickProgram=new ShaderProgram(gl,surfaceVertex,pickFragment);this.background=new ShaderProgram(gl,quadVertex,backgroundFragment);this.blur=new ShaderProgram(gl,quadVertex,blurFragment);this.composite=new ShaderProgram(gl,quadVertex,compositeFragment);this.emptyVao=gl.createVertexArray();this.targets=[];this.draws=0;this.shadowSize=1024;
      this.shadowFbo=gl.createFramebuffer();this.shadowTexture=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,this.shadowTexture);gl.texImage2D(gl.TEXTURE_2D,0,gl.DEPTH_COMPONENT24,this.shadowSize,this.shadowSize,0,gl.DEPTH_COMPONENT,gl.UNSIGNED_INT,null);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);gl.bindFramebuffer(gl.FRAMEBUFFER,this.shadowFbo);gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.TEXTURE_2D,this.shadowTexture,0);gl.drawBuffers([gl.NONE]);gl.readBuffer(gl.NONE);if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE)throw new Error('阴影帧缓冲创建失败');gl.bindFramebuffer(gl.FRAMEBUFFER,null);
    }
    resize(){const g=this.gl,rect=this.canvas.getBoundingClientRect(),dpr=Math.min(window.devicePixelRatio||1,1.5),w=Math.max(1,Math.floor(rect.width*dpr)),h=Math.max(1,Math.floor(rect.height*dpr));if(this.canvas.width===w&&this.canvas.height===h&&this.targets.length)return;this.canvas.width=w;this.canvas.height=h;this.targets.forEach(t=>t.dispose());this.color=new RenderTarget(g,w,h,true);this.pickTarget=new RenderTarget(g,w,h,true);this.bloomA=new RenderTarget(g,Math.max(1,Math.floor(w/3)),Math.max(1,Math.floor(h/3)));this.bloomB=new RenderTarget(g,this.bloomA.w,this.bloomA.h);this.targets=[this.color,this.pickTarget,this.bloomA,this.bloomB];}
    texture(unit,texture){const g=this.gl;g.activeTexture(g.TEXTURE0+unit);g.bindTexture(g.TEXTURE_2D,texture);}
    quad(){const g=this.gl;g.bindVertexArray(this.emptyVao);g.drawArrays(g.TRIANGLES,0,3);this.draws++;}
    common(p,model,view,proj,camera,expand=0){p.use().mat4('uModel',model).mat4('uView',view).mat4('uProjection',proj).mat4('uLightVP',this.lightVP).float('uExpand',expand).vec3('uCamera',camera.position);}
    render(scene,camera,state){
      const g=this.gl;this.resize();this.draws=0;const objects=scene.objects();this.triangles=objects.reduce((s,o)=>s+o.part.mesh.triangles,0);const view=camera.view(),proj=camera.projection(this.canvas.width/this.canvas.height);const angle=state.lightAngle*Math.PI/180;this.light=V.norm([Math.sin(angle)*7,9,Math.cos(angle)*7]);this.lightVP=Mat4.multiply(Mat4.ortho(-9,9,-9,9,1,32),Mat4.lookAt(V.mul(this.light,15),[0,0,0],[0,1,0]));
      g.enable(g.DEPTH_TEST);g.depthMask(true);g.depthFunc(g.LESS);g.enable(g.CULL_FACE);g.cullFace(g.BACK);g.disable(g.BLEND);
      if(state.shadow&&state.style!=='wireframe'){
        g.bindFramebuffer(g.FRAMEBUFFER,this.shadowFbo);g.viewport(0,0,this.shadowSize,this.shadowSize);g.clear(g.DEPTH_BUFFER_BIT);g.enable(g.POLYGON_OFFSET_FILL);g.polygonOffset(1.5,2);this.depthProgram.use().mat4('uLightVP',this.lightVP);
        for(const o of objects){if(o.part.mesh===scene.meshes.plane)continue;this.depthProgram.mat4('uModel',o.model);o.part.mesh.draw();this.draws++;}g.disable(g.POLYGON_OFFSET_FILL);
      }
      this.color.bind();g.clearColor(0,0,0,1);g.clear(g.COLOR_BUFFER_BIT|g.DEPTH_BUFFER_BIT);g.disable(g.DEPTH_TEST);g.disable(g.CULL_FACE);this.background.use().int('uStyle',styleInfo[state.style].id);this.quad();g.enable(g.DEPTH_TEST);g.enable(g.CULL_FACE);g.cullFace(g.BACK);
      this.texture(2,this.shadowTexture);
      for(const o of objects){const {entity:e,part}=o,p=this.pipeline.get(state.style),m=part.material;
        if(state.style==='toon'&&e.id!==0){g.cullFace(g.FRONT);this.common(this.pipeline.outline,o.model,view,proj,camera,.026);this.pipeline.outline.vec3('uOutline',e.id===scene.selectedId?[.20,.13,.29]:[.13,.12,.21]);part.mesh.draw();this.draws++;g.cullFace(g.BACK);}
        this.common(p,o.model,view,proj,camera);p.vec3('uColor',hexRGB(m.theme?e.color:m.color)).vec3('uLight',this.light).float('uMetallic',m.theme?e.metal:m.metal).float('uRoughness',m.theme?e.rough:m.rough).float('uEmission',m.emission).float('uExposure',state.exposure).float('uTime',state.time).float('uShadow',state.shadow?1:0).float('uGrid',state.grid&&part.mesh===scene.meshes.plane?1:0).float('uGround',part.mesh===scene.meshes.plane?1:0).float('uEnvironment',e.id===0&&m.emission===0?1:0).int('uShadowMap',2);
        if(state.style==='wireframe'){g.enable(g.POLYGON_OFFSET_FILL);g.polygonOffset(1,1);part.mesh.draw();g.disable(g.POLYGON_OFFSET_FILL);this.common(this.pipeline.outline,o.model,view,proj,camera);this.pipeline.outline.vec3('uOutline',e.id===scene.selectedId?[.85,.71,1.]:[.34,.61,.7]);part.mesh.draw(true);this.draws+=2;}else{part.mesh.draw();this.draws++;}
      }
      g.disable(g.DEPTH_TEST);g.disable(g.CULL_FACE);
      if(state.bloom){
        for(let i=0;i<4;i++){const target=i%2===0?this.bloomA:this.bloomB,source=i===0?this.color:(i%2===0?this.bloomB:this.bloomA);target.bind();this.blur.use().int('uSource',0).float('uExtract',i===0?1:0).vec2('uDirection',i%2===0?[1/source.w,0]:[0,1/source.h]);this.texture(0,source.texture);this.quad();}
      }
      g.bindFramebuffer(g.FRAMEBUFFER,null);g.viewport(0,0,this.canvas.width,this.canvas.height);this.composite.use().int('uSource',0).int('uBloom',1).float('uBloomStrength',state.bloom?(state.style==='neon'?.95:.25):0);this.texture(0,this.color.texture);this.texture(1,this.bloomB.texture);this.quad();g.bindVertexArray(null);
    }
    pick(scene,camera,x,y){const g=this.gl;this.pickTarget.bind();g.disable(g.BLEND);g.disable(g.DITHER);g.enable(g.DEPTH_TEST);g.enable(g.CULL_FACE);g.cullFace(g.BACK);g.clearColor(0,0,0,1);g.clear(g.COLOR_BUFFER_BIT|g.DEPTH_BUFFER_BIT);const view=camera.view(),proj=camera.projection(this.canvas.width/this.canvas.height);for(const o of scene.objects()){this.common(this.pickProgram,o.model,view,proj,camera);this.pickProgram.float('uId',o.entity.id);o.part.mesh.draw();}const px=new Uint8Array(4);g.readPixels(clamp(x,0,this.canvas.width-1),clamp(y,0,this.canvas.height-1),1,1,g.RGBA,g.UNSIGNED_BYTE,px);g.enable(g.DITHER);g.bindFramebuffer(g.FRAMEBUFFER,null);return px[0];}
  }

  const canvas=$('glCanvas'),camera=new Camera(),keys=new Set();
  const state={style:'toon',scene:'courier',lightAngle:35,exposure:1.1,shadow:true,bloom:false,grid:false,turntable:false,hud:true,time:0,fps:0,frameMs:0};
  let gl,renderer,scene,toastTimer,last=0,frames=0,measure=0,drawCounter=0,ready=false;
  function toast(message){$('toast').textContent=message;$('toast').classList.add('visible');clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('toast').classList.remove('visible'),2300);}
  function fail(error){console.error(error);const box=$('errorBox');box.hidden=false;box.textContent='渲染器未能启动：'+error.message+'。请使用支持 WebGL2 的 Chrome、Safari 或 Edge 打开此页面。';$('renderStatus').textContent='渲染器初始化失败';canvas.dataset.ready='false';}
  function updateSceneTree(){const tree=$('sceneTree');tree.replaceChildren();scene.entities.forEach(e=>{const b=document.createElement('button');b.className='entity';b.setAttribute('aria-selected',e.id===scene.selectedId?'true':'false');b.dataset.entity=String(e.id);b.innerHTML='<span class="icon">◇</span><span></span><span class="visibility">◉</span>';b.children[1].textContent=e.name;b.addEventListener('click',()=>selectEntity(e.id));tree.append(b);});$('entityCount').textContent=scene.entities.length+' OBJECTS';$('sceneRoot').textContent=sceneInfo[state.scene].title;}
  function selectEntity(id){if(!scene.entities.some(e=>e.id===id))return;scene.selectedId=id;updateSceneTree();syncInspector();canvas.dataset.selected=scene.selected.name;}
  function syncInspector(){const e=scene.selected;$('selectedName').textContent=e.name;$('selectedType').textContent=e.type;$('baseColor').value=e.color;$('colorValue').textContent=e.color;$('metallic').value=e.metal;$('metallicValue').textContent=e.metal.toFixed(2);$('roughness').value=e.rough;$('roughnessValue').textContent=e.rough.toFixed(2);$('objectRotation').value=e.rotation;$('rotationValue').textContent=e.rotation+'°';$('objectScale').value=e.scale;$('scaleValue').textContent=e.scale.toFixed(2)+'×';}
  function setStyle(name,notify=true){if(!styleInfo[name])return;state.style=name;state.bloom=name==='neon';$('bloomToggle').checked=state.bloom;document.querySelectorAll('.style-button[data-style]').forEach(b=>{const active=b.dataset.style===name;b.classList.toggle('active',active);b.setAttribute('aria-pressed',String(active));});$('styleValue').textContent=styleInfo[name].name;$('stylePill').textContent=styleInfo[name].tag;$('viewport').className='viewport theme-'+name;canvas.dataset.style=name;document.title=styleInfo[name].name+' · 光场工坊';if(notify)toast('已切换为 '+styleInfo[name].name);}
  function setScene(name){if(!sceneInfo[name])return;state.scene=name;scene.create(name);camera.reset(name);state.turntable=false;syncTurntable();updateSceneTree();syncInspector();const info=sceneInfo[name];$('sceneTitle').textContent=info.title;$('sceneSubtitle').textContent=info.subtitle;$('sceneDescription').textContent=info.description;document.querySelectorAll('.preset[data-scene]').forEach(b=>{const a=b.dataset.scene===name;b.classList.toggle('active',a);b.setAttribute('aria-pressed',String(a));});canvas.dataset.scene=name;canvas.dataset.selected=scene.selected.name;}
  function resetScene(){state.lightAngle=35;state.exposure=1.1;state.shadow=true;state.grid=false;state.time=0;$('lightAngle').value=35;$('lightAngleValue').textContent='35°';$('exposure').value=1.1;$('exposureValue').textContent='1.10×';$('shadowToggle').checked=true;$('gridToggle').checked=false;setScene(state.scene);setStyle('toon',false);toast('已恢复当前场景的初始状态');}
  function syncTurntable(){$('turntableButton').setAttribute('aria-pressed',String(state.turntable));$('turntableButton').textContent=state.turntable?'Ⅱ 暂停转台':'▷ 自动转台';}
  function toggleHud(){state.hud=!state.hud;$('viewHeading').hidden=!state.hud;$('viewportHints').style.display=state.hud?'flex':'none';$('hudToggle').setAttribute('aria-pressed',String(state.hud));}
  function updateStats(){const p=camera.position;$('fpsValue').textContent=state.fps.toFixed(0);$('drawValue').textContent=renderer.draws;$('triangleValue').textContent=(renderer.triangles/1000).toFixed(1);$('frameValue').textContent=state.frameMs.toFixed(1)+' ms';$('cameraValue').textContent=p.map(x=>x.toFixed(1)).join(', ');$('renderStatus').textContent=styleInfo[state.style].tag+' · '+scene.entities.length+' 个可编辑对象';canvas.dataset.camera=p.map(x=>x.toFixed(3)).join(',');canvas.dataset.fps=state.fps.toFixed(1);canvas.dataset.draws=String(renderer.draws);canvas.dataset.selected=scene.selected.name;}
  function frame(now){try{if(!last){last=now;measure=now;}const dt=Math.min((now-last)/1000,.05);last=now;state.time+=dt;frames++;camera.update(dt,keys);if(state.turntable)camera.yaw+=dt*.24;renderer.render(scene,camera,state);drawCounter++;if(now-measure>500){state.fps=frames*1000/(now-measure);state.frameMs=1000/state.fps;frames=0;measure=now;updateStats();}if(drawCounter===2){ready=true;canvas.dataset.ready='true';updateStats();}requestAnimationFrame(frame);}catch(e){fail(e);}}

  try{
    gl=canvas.getContext('webgl2',{antialias:true,alpha:false,preserveDrawingBuffer:true});if(!gl)throw new Error('浏览器没有提供 WebGL2 上下文');
    const meshes={};for(const name of['cube','plane','sphere','cylinder','torus'])meshes[name]=MeshFactory[name](gl);
    scene=new Scene(meshes);renderer=new Renderer(gl,canvas);setScene('courier');setStyle('toon',false);$('glInfo').textContent='WebGL 2.0 · 全部素材在本地生成 · 产品原型 0.2';requestAnimationFrame(frame);
  }catch(e){fail(e);return;}

  document.querySelectorAll('.style-button[data-style]').forEach(b=>b.addEventListener('click',()=>setStyle(b.dataset.style)));
  document.querySelectorAll('.preset[data-scene]').forEach(b=>b.addEventListener('click',()=>setScene(b.dataset.scene)));
  $('baseColor').addEventListener('input',e=>{scene.selected.color=e.target.value;$('colorValue').textContent=e.target.value;});
  document.querySelectorAll('[data-color]').forEach(b=>b.addEventListener('click',()=>{scene.selected.color=b.dataset.color;syncInspector();}));
  for(const [id,key,out,format]of[['metallic','metal','metallicValue',v=>v.toFixed(2)],['roughness','rough','roughnessValue',v=>v.toFixed(2)],['objectRotation','rotation','rotationValue',v=>v+'°'],['objectScale','scale','scaleValue',v=>v.toFixed(2)+'×']])$(id).addEventListener('input',e=>{const value=Number(e.target.value);scene.selected[key]=value;$(out).textContent=format(value);});
  for(const [id,key,out,format]of[['lightAngle','lightAngle','lightAngleValue',v=>v+'°'],['exposure','exposure','exposureValue',v=>v.toFixed(2)+'×']])$(id).addEventListener('input',e=>{const value=Number(e.target.value);state[key]=value;$(out).textContent=format(value);});
  for(const [id,key]of[['shadowToggle','shadow'],['bloomToggle','bloom'],['gridToggle','grid']])$(id).addEventListener('change',e=>state[key]=e.target.checked);
  $('turntableButton').addEventListener('click',()=>{state.turntable=!state.turntable;syncTurntable();});$('focusButton').addEventListener('click',()=>{camera.reset(state.scene);toast('已适配整个场景');});$('resetButton').addEventListener('click',resetScene);$('hudToggle').addEventListener('click',toggleHud);
  $('helpButton').addEventListener('click',()=>{keys.clear();$('helpDialog').showModal();});$('closeHelp').addEventListener('click',()=>$('helpDialog').close());
  $('exportButton').addEventListener('click',()=>{renderer.render(scene,camera,state);const a=document.createElement('a');a.download=`LUMA-${state.scene}-${state.style}.png`;a.href=canvas.toDataURL('image/png');a.click();toast('已导出当前画面的 PNG 图片');});
  window.addEventListener('keydown',e=>{if($('helpDialog').open||['INPUT','TEXTAREA','SELECT'].includes(document.activeElement.tagName))return;if(['Space','F1','KeyW','KeyA','KeyS','KeyD','KeyQ','KeyE'].includes(e.code))e.preventDefault();keys.add(e.code);if(e.repeat)return;const style=['Digit1','Digit2','Digit3','Digit4'].indexOf(e.code);if(style!==-1)setStyle(['realistic','toon','neon','wireframe'][style]);if(e.code==='KeyR')resetScene();if(e.code==='F1')toggleHud();if(e.code==='Space'){state.turntable=!state.turntable;syncTurntable();}if(e.key==='?'){$('helpDialog').showModal();keys.clear();}});
  window.addEventListener('keyup',e=>keys.delete(e.code));window.addEventListener('blur',()=>keys.clear());document.addEventListener('visibilitychange',()=>{if(document.hidden)keys.clear();last=0;measure=performance.now();frames=0;});
  let drag=null;
  canvas.addEventListener('contextmenu',e=>e.preventDefault());
  canvas.addEventListener('pointerdown',e=>{if(e.button!==0&&e.button!==2)return;canvas.focus({preventScroll:true});drag={x:e.clientX,y:e.clientY,startX:e.clientX,startY:e.clientY,pan:e.button===2||e.shiftKey,pointerId:e.pointerId};canvas.classList.add('dragging');canvas.setPointerCapture(e.pointerId);});
  canvas.addEventListener('pointermove',e=>{if(!drag)return;const dx=e.clientX-drag.x,dy=e.clientY-drag.y;if(drag.pan)camera.pan(dx,dy);else camera.orbit(dx,dy);drag.x=e.clientX;drag.y=e.clientY;});
  canvas.addEventListener('pointerup',e=>{if(!drag)return;const moved=Math.hypot(e.clientX-drag.startX,e.clientY-drag.startY);if(moved<4&&!drag.pan){const rect=canvas.getBoundingClientRect(),x=Math.floor((e.clientX-rect.left)/rect.width*canvas.width),y=Math.floor((rect.bottom-e.clientY)/rect.height*canvas.height);const id=renderer.pick(scene,camera,x,y);if(id)selectEntity(id);}drag=null;canvas.classList.remove('dragging');if(canvas.hasPointerCapture(e.pointerId))canvas.releasePointerCapture(e.pointerId);});
  canvas.addEventListener('pointercancel',()=>{drag=null;canvas.classList.remove('dragging');});
  canvas.addEventListener('wheel',e=>{e.preventDefault();camera.distance=clamp(camera.distance*Math.exp(e.deltaY*.001),4.5,25);},{passive:false});
  canvas.addEventListener('webglcontextlost',e=>{e.preventDefault();ready=false;fail(new Error('GPU 上下文已丢失，请刷新页面恢复'));});
  window.visualWorkbench={setStyle,setScene,resetScene,getState:()=>({...state,ready,camera:[...camera.position],selected:scene.selected.name,objectCount:scene.entities.length,draws:renderer.draws,glVersion:'WebGL2',glError:gl.getError()})};
})();
