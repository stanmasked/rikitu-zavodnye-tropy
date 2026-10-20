/* Compare native MAP02 data against the authoritative browser game.js. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const native=fs.readFileSync(path.join(root,'native/rikitu_mill_data.h'),'utf8');
const st=game.indexOf("{name:'Мельница у ручья'");
const en=game.indexOf("{name:'Грибная роща'",st);
assert(st>=0&&en>st);
const chunk=game.slice(st,en);
function array(k,n){
  const m=chunk.match(new RegExp(k+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+n+':'));
  assert(m,'Missing browser '+k);return JSON.parse(m[1]);
}
function parseData(n,width){
  const m=native.match(new RegExp('static const \\w+ '+n+'\\[\\d+\\] = \\{([\\s\\S]*?)\\};'));
  assert(m,'Missing C99 data '+n);
  return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(row=>{
    const values=row[1].split(',').map(z=>Number(z.trim().replace(/f$/,'')));
    assert.equal(values.length,width);return values;
  });
}
const ground=array('ground','platforms'),platforms=array('platforms','fragments');
const fragments=array('fragments','checkpoints'),checkpoints=array('checkpoints','enemies');
const enemies=[...chunk.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
  .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
  ...platforms.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
  for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of platforms)
  for(const ratio of [.25,.5,.75])coins.push([Math.round(x+w*ratio),y-46]);
let i=0;
while(coins.length<90){
  const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
  if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))
    coins.push([x,425]);
  if(++i>5000)throw Error('coin generation diverged');
}
const cp=native.match(/static const float rikitu_mill_checkpoints\[3\]=\{([^}]+)\};/);
assert(cp);
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(enemies.length,6);
assert.equal(coins.length,90);
assert.deepEqual(parseData('rikitu_mill_solids',4),solids);
assert.deepEqual(parseData('rikitu_mill_fragments',2),fragments);
assert.deepEqual(parseData('rikitu_mill_coins',2),coins);
assert.deepEqual(parseData('rikitu_mill_enemy_spawns',8),enemies);
assert.deepEqual(cp[1].split(',').map(v=>Number(v.trim().replace(/f$/,''))),checkpoints);
const boss=chunk.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert(boss);
assert.deepEqual(boss.slice(1),['7910','3','Заводной Жук']);
assert.equal(Number(chunk.match(/width:(\d+)/)[1]),8550);
assert.equal(Number(chunk.match(/gate:(\d+)/)[1]),8370);
console.log('PASS MAP02 browser-source parity: five ground spans, thirteen platforms, ninety coins, three fragments, three checkpoints, six enemies, boss x7910 HP3, gate8370');
