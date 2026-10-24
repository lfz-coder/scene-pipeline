#!/usr/bin/env python3
"""Generate a self-contained Chinese blog reader with Python's standard library."""
from pathlib import Path
import base64
import json
import mimetypes
import os
import re
import tomllib
from urllib.parse import unquote, quote

ROOT = Path(__file__).resolve().parents[1]

def script_json(value):
    return json.dumps(value, ensure_ascii=False).replace('<', '\\u003c').replace('>', '\\u003e').replace('&', '\\u0026')

def prepare_body(body, article_dir, output_dir):
    # C++ captures such as [mat](auto& c) are source code, not Markdown links.
    protected = {}
    def protect(match):
        token = f'\x00CODE{len(protected)}\x00'
        protected[token] = match.group(0)
        return token
    body = re.sub(r'(?ms)^[ \t]*(`{3,}|~{3,})[^\n]*\n.*?^[ \t]*\1[ \t]*(?=\n|\Z)',
                  protect, body)
    body = re.sub(r'(`+)([^\n]*?)\1', protect, body)
    def image(match):
        alt, target = match.groups()
        if target.startswith(('https://', 'http://', 'data:')):
            raise ValueError(f'Article image must be local: {target}')
        path = (article_dir / unquote(target)).resolve()
        mime = mimetypes.guess_type(path.name)[0] or 'application/octet-stream'
        data = base64.b64encode(path.read_bytes()).decode()
        return f'![{alt}](data:{mime};base64,{data})'
    body = re.sub(r'!\[([^\]]*)\]\(([^)]+)\)', image, body)
    def link(match):
        label, target = match.groups()
        if target.startswith(('https://', 'http://', 'data:', '#', 'mailto:')):
            return match.group(0)
        path_part, separator, fragment = target.partition('#')
        resolved = (article_dir / unquote(path_part)).resolve()
        if not resolved.exists():
            raise ValueError(f'Broken link: {target} in {article_dir}')
        if resolved.name == 'index.zh.md' and resolved.parent.parent == ROOT/'articles':
            return f'[{label}](#chapter-{resolved.parent.name})'
        relative = quote(os.path.relpath(resolved, output_dir.resolve()), safe='/.-_')
        return f'[{label}]({relative}{separator}{fragment})'
    body = re.sub(r'(?<!!)\[([^\]]*)\]\(([^)]+)\)', link, body)
    for token, code in protected.items():
        body = body.replace(token, code)
    return body

def load_articles():
    result=[]
    for path in sorted((ROOT/'articles').glob('0*/index.zh.md')):
        text=path.read_text()
        if not text.startswith('+++\n'): raise ValueError(f'Missing front matter: {path}')
        _,front,body=text.split('+++',2)
        meta=tomllib.loads(front)
        result.append({'id':path.parent.name,'title':meta['title'],'description':meta['description'],
                       'body':prepare_body(body,path.parent,ROOT/'articles'),
                       'source':quote(f'{path.parent.name}/{path.name}')})
    if not result: raise ValueError('No article chapters found')
    return result

SHELL='''<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>光场工坊 · OpenGL 开发手记</title>
<style>
:root{--bg:#0f1219;--panel:#171c27;--text:#dce4f5;--muted:#929fb9;--line:#2a3448;--accent:#b5a0f7;--code:#101620;--card:#1e2635;color-scheme:dark}*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.9 -apple-system,BlinkMacSystemFont,"PingFang SC",system-ui,sans-serif}body.light{--bg:#f5f4f1;--panel:#fff;--text:#283044;--muted:#697388;--line:#dedfe4;--accent:#6e51b1;--code:#eceef3;--card:#f1eff7;color-scheme:light}a{color:var(--accent);text-decoration:none}a:hover{text-decoration:underline}button{font:inherit;cursor:pointer}header{height:74px;border-bottom:1px solid var(--line);display:flex;align-items:center;justify-content:space-between;padding:0 34px;background:var(--panel);position:sticky;top:0;z-index:4}.brand{font-size:18px;font-weight:750}.brand small{font-size:10px;color:var(--muted);font-weight:450;display:block;line-height:1.2;letter-spacing:2px}.actions{display:flex;gap:22px;align-items:center;font-size:12px}#theme{border:1px solid var(--line);background:var(--card);border-radius:8px;color:var(--text);padding:4px 12px}.layout{display:grid;grid-template-columns:260px minmax(0,1fr) 210px;max-width:1580px;margin:auto;min-height:calc(100vh - 74px)}aside{padding:36px 22px}.chapters{border-right:1px solid var(--line);position:sticky;top:74px;height:calc(100vh - 74px);overflow:auto}.eyebrow{font-size:11px;letter-spacing:2px;color:var(--muted);margin:0 0 18px}.chapter{display:block;text-align:left;width:100%;border:1px solid transparent;color:var(--muted);background:transparent;border-radius:10px;padding:15px 12px;margin-bottom:10px;line-height:1.65;font-size:13px}.chapter strong{display:block;font-size:11px;letter-spacing:1px;margin-bottom:7px}.chapter[aria-current=true]{color:var(--text);background:var(--card);border-color:var(--line)}.chapter[aria-current=true] strong{color:var(--accent)}.note{color:var(--muted);font-size:12px;margin-top:36px}.content{padding:44px 56px 90px;min-width:0;max-width:980px}.page-top{display:flex;justify-content:space-between;gap:12px;font-size:11px;color:var(--muted);margin-bottom:18px}#article-title{font-size:36px;line-height:1.45;letter-spacing:-.7px;margin:14px 0 20px}#description{font-size:16px;color:var(--muted);line-height:1.8;margin-bottom:28px;padding-bottom:25px;border-bottom:1px solid var(--line)}article{overflow-wrap:break-word}article h1{display:none}article h2{font-size:24px;line-height:1.6;margin:44px 0 17px;scroll-margin-top:100px}article h3{font-size:18px;margin:30px 0 12px;scroll-margin-top:100px}article p{margin:14px 0 20px}article img{max-width:100%;height:auto;border-radius:12px;border:1px solid var(--line);display:block;margin:24px auto}article blockquote{border-left:3px solid var(--accent);margin:25px 0;padding:1px 21px;background:var(--card);color:var(--muted);border-radius:0 8px 8px 0}article pre{background:var(--code);border:1px solid var(--line);border-radius:10px;padding:24px 18px 18px;overflow:auto;line-height:1.7;font-size:12.5px;position:relative}article code{font-family:ui-monospace,SFMono-Regular,Consolas,monospace}article :not(pre)>code{background:var(--card);padding:2px 5px;border-radius:4px;font-size:.88em}article table{width:100%;border-collapse:collapse;display:block;overflow:auto;font-size:13px;margin:25px 0}article th,article td{border:1px solid var(--line);padding:11px 13px;text-align:left}article th{background:var(--card);font-weight:650}article ul,article ol{padding-left:25px}article li{margin:7px 0}article hr{border:0;border-top:1px solid var(--line);margin:36px 0}.toc{position:sticky;top:74px;height:calc(100vh - 74px);overflow:auto;font-size:12px;padding-left:10px}.toc a{display:block;color:var(--muted);line-height:1.6;padding:6px 0}.copy{position:absolute;right:10px;top:6px;border:0;background:var(--card);color:var(--muted);border-radius:4px;font:10px system-ui;padding:3px 7px}.pager{border-top:1px solid var(--line);margin-top:44px;padding-top:23px;display:flex;justify-content:space-between;gap:16px}.pager button{background:var(--card);border:1px solid var(--line);border-radius:9px;color:var(--text);padding:10px 16px;font-size:13px}.pager button:disabled{opacity:.3;cursor:default}footer{font-size:11px;color:var(--muted);padding-top:28px}@media(max-width:1180px){.layout{grid-template-columns:230px minmax(0,1fr)}.toc{display:none}.content{padding:32px 35px}}@media(max-width:740px){header{padding:0 18px;height:65px}.actions{gap:12px}.layout{display:block}.chapters{position:static;height:auto;border-right:0;border-bottom:1px solid var(--line);padding:12px}.chapters nav{display:flex;gap:5px;overflow:auto}.chapters .eyebrow,.note{display:none}.chapter{min-width:170px;max-width:200px;margin:0;font-size:11px;padding:10px}.content{padding:22px 20px 60px}#article-title{font-size:27px}.actions>a{font-size:10px}article h2{font-size:22px}.page-top{font-size:9px}article pre{font-size:11px}}
</style></head><body><header><a href="../index.html" class="brand">◈ 光场工坊<small>光场工坊 · 开发手记</small></a><div class="actions"><a href="../index.html">体验产品 Demo ↗</a><a href="../native/README.md">运行 C++ 工程</a><button id="theme" aria-label="切换阅读主题">☀ 浅色</button></div></header>
<div class="layout"><aside class="chapters"><div class="eyebrow">从 OPENGL 到软件</div><nav id="chapters"></nav><div class="note">第一、二阶段 · 可复用工作台<br>完整中文教程 / 实际源码<br><br>无需 CDN，内容和配图已内嵌。源码链接指向同目录工程。</div></aside><main class="content"><div class="page-top"><span id="chapter-label"></span><a id="markdown-source">Markdown 原稿 ↗</a></div><h1 id="article-title"></h1><p id="description"></p><article id="body"></article><div class="pager"><button id="previous">← 上一篇</button><button id="next">下一篇 →</button></div><footer>光场工坊 · 第一、二阶段实现记录。模型、阴影与 PBR 随后续学习阶段扩展。</footer></main><aside class="toc"><div class="eyebrow">本篇目录</div><nav id="toc"></nav></aside></div>
<script>/* MARKED_LIBRARY */</script><script id="article-data" type="application/json">/* ARTICLE_DATA */</script><script>
'use strict';
const articles=JSON.parse(document.getElementById('article-data').textContent);let current=0;const byId=id=>document.getElementById(id);
function render(index,scroll=true){current=Math.max(0,Math.min(index,articles.length-1));const item=articles[current];byId('article-title').textContent=item.title;byId('description').textContent=item.description;byId('chapter-label').textContent=`第 ${String(current+1).padStart(2,'0')} 篇 / 共 ${String(articles.length).padStart(2,'0')} 篇 · C++17 / OPENGL 3.3`;byId('markdown-source').href=item.source;byId('body').innerHTML=marked.parse(item.body,{gfm:true});byId('chapters').replaceChildren();articles.forEach((a,i)=>{const b=document.createElement('button');b.className='chapter';b.setAttribute('aria-current',String(i===current));const n=document.createElement('strong');n.textContent=`${String(i+1).padStart(2,'0')} · 开发手记`;b.append(n,document.createTextNode(a.title));b.onclick=()=>render(i);byId('chapters').append(b);});byId('toc').replaceChildren();byId('body').querySelectorAll('h2').forEach((h,i)=>{h.id=`section-${current}-${i}`;const a=document.createElement('a');a.href='#'+h.id;a.textContent=h.textContent;byId('toc').append(a);});byId('body').querySelectorAll('pre').forEach(pre=>{const b=document.createElement('button');b.className='copy';b.textContent='复制';b.onclick=async()=>{try{await navigator.clipboard.writeText(pre.querySelector('code').textContent);b.textContent='已复制';}catch{b.textContent='请选中复制';}};pre.append(b);});byId('previous').disabled=current===0;byId('next').disabled=current===articles.length-1;document.title=item.title+' · 光场工坊';byId('body').dataset.ready='true';byId('body').dataset.chapter=item.id;if(scroll){history.replaceState(null,'','#chapter-'+item.id);window.scrollTo({top:0,behavior:'instant'});}}
byId('previous').onclick=()=>render(current-1);byId('next').onclick=()=>render(current+1);byId('theme').onclick=()=>{const light=document.body.classList.toggle('light');byId('theme').textContent=light?'☾ 深色':'☀ 浅色';};function chapterFromHash(){const id=location.hash.slice(9);return articles.findIndex(a=>a.id===id);}window.addEventListener('hashchange',()=>{if(location.hash.startsWith('#chapter-')){const index=chapterFromHash();if(index>=0)render(index);}});const initial=chapterFromHash();render(initial>=0?initial:0,false);
</script></body></html>'''

def sync_reuse_example():
    article=ROOT/'articles/03-scene-and-camera/index.zh.md'
    text=article.read_text()
    for heading, filename in [
        ('## 可编译示例源码实录', 'reusable_scene.cpp'),
        ('## 无窗口控制器源码实录', 'controller_example.cpp'),
    ]:
        source=(ROOT/'native/examples'/filename).read_text().rstrip()
        start=text.index('```cpp',text.index(heading))
        end=text.index('```',start+6)+3
        text=text[:start]+'```cpp\n'+source+'\n```'+text[end:]
    article.write_text(text)

def main():
    sync_reuse_example()
    sync_article_sources()
    articles=load_articles()
    library=(ROOT/'articles/vendor/marked.umd.js').read_text()
    if '</script' in library.lower(): raise ValueError('Renderer contains script terminator')
    result=SHELL.replace('/* MARKED_LIBRARY */',library).replace('/* ARTICLE_DATA */',script_json(articles))
    output=ROOT/'articles/index.html';output.write_text(result)
    print(f'Built {output}: {len(articles)} chapters, {output.stat().st_size:,} bytes.')

def sync_article_sources():
    pattern = r'```(cpp|glsl)\n// SOURCE: ([^\n]+)\n.*?```'
    def source(match):
        language, relative = match.group(1), match.group(2)
        content = (ROOT/relative).read_text().rstrip()
        return f'```{language}\n// SOURCE: {relative}\n{content}\n```'
    for chapter in ('05-stage-two-lighting', '06-studio-pipeline'):
        article = ROOT/'articles'/chapter/'index.zh.md'
        article.write_text(re.sub(pattern, source, article.read_text(), flags=re.S))

if __name__=='__main__': main()
