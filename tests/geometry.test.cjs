const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const app = fs.readFileSync(path.join(__dirname, '../workbench.js'), 'utf8');
const source = app.slice(app.indexOf('const MeshFactory ='), app.indexOf('class ShaderProgram'));
const context = { Mesh: class { constructor(gl, vertices, indices) { this.vertices = vertices; this.indices = indices; } } };
vm.createContext(context);
vm.runInContext(source + '; globalThis.factory = MeshFactory;', context);

// A visible outward triangle must have the same orientation as its vertex normals.
// This catches geometry which vanishes with back-face culling or looks inside-out.
for (const name of ['cube', 'plane', 'sphere', 'cylinder', 'torus']) {
  const {vertices: v, indices: ix} = context.factory[name](null);
  let triangles = 0;
  for (let i = 0; i < ix.length; i += 3) {
    const a = ix[i] * 10, b = ix[i+1] * 10, c = ix[i+2] * 10;
    const u = [v[b]-v[a], v[b+1]-v[a+1], v[b+2]-v[a+2]];
    const w = [v[c]-v[a], v[c+1]-v[a+1], v[c+2]-v[a+2]];
    const face = [u[1]*w[2]-u[2]*w[1], u[2]*w[0]-u[0]*w[2], u[0]*w[1]-u[1]*w[0]];
    if (Math.hypot(...face) < 1e-9) continue; // UV sphere pole degenerates.
    const facing = face.reduce((sum, x, axis) => sum + x * (v[a+3+axis] + v[b+3+axis] + v[c+3+axis]), 0);
    assert(facing > 0, `${name}: triangle ${i/3} winds against its normals (${facing})`);
    triangles++;
  }
  assert(triangles > 0);
  console.log(`${name}: ${triangles} outward triangles OK`);
}
