// Embed the development source so index.html can be opened or shared on its own.
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..');
const entry = path.join(root, 'index.html');
const source = fs.readFileSync(path.join(root, 'workbench.js'), 'utf8');
const html = fs.readFileSync(entry, 'utf8');
const script = /<script(?: src="workbench\.js"| data-workbench)>[\s\S]*?<\/script>/;
if (!script.test(html)) throw new Error('Cannot find the workbench script slot');
fs.writeFileSync(entry, html.replace(script, () => '<script data-workbench>\n' + source.replace(/<\/script/gi, '<\\/script') + '\n</script>'));
console.log('Built standalone index.html (' + fs.statSync(entry).size + ' bytes)');
