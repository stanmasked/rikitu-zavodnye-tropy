/* Every native MAP04 coordinate must match live browser game.js. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const h=fs.readFileSync(path.join(root,'native/rikitu_bridge_data.h'),'utf8');
const start=game.indexOf("{name:'Мост шестерён'");
const end=game.indexOf("{name:'Улица мастерских'",start);
assert(start>=0&&end>start);
const chunk=game.slice(start,end);
function arr(key,next){
 const m=chunk.match(new RegExp(key+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing source '+key);
 return JSON.parse(m[1]);
}
function cdata(name,arity){
 const m=h.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'Missing C array '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(q=>{
  const r=q[1].split(',').map(z=>Number(z.trim().replace(/f$/,'')));
  assert.equal(r.length,arity,name);return r;
 });
}
const ground=arr('ground','platforms'),platforms=arr('platforms','switches');
const fragments=arr('fragments','checkpoints'),cp=arr('checkpoints','enemies');
const switches=[...chunk.matchAll(/\[(\d+),GROUND\]/g)].map(m=>[+m[1],456,0]);
const enemies=[...chunk.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
 ...platforms.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
 for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of platforms)
 for(const t of [.25,.50,.75])coins.push([Math.round(x+w*t),y-46]);
let i=0;
while(coins.length<90){
 const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
 if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))coins.push([x,425]);
 if(++i>5000)throw Error('coins failed');
}
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(enemies.length,6);
assert.equal(coins.length>=90,true);
assert.deepEqual(cdata('rikitu_bridge_solids',4),solids);
assert.deepEqual(cdata('rikitu_bridge_fragments',2),fragments);
assert.deepEqual(cdata('rikitu_bridge_coins',2),coins.slice(0,90));
assert.deepEqual(cdata('rikitu_bridge_enemies',8),enemies);
assert.deepEqual(cdata('rikitu_bridge_levers',3),switches);
const cps=h.match(/static const float rikitu_bridge_checkpoints\[3\]=\{([^}]+)\};/);
assert(cps);
assert.deepEqual(cps[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),cp);
assert.equal(Number(chunk.match(/width:(\d+)/)[1]),8880);
assert.equal(Number(chunk.match(/gate:(\d+)/)[1]),8740);
const boss=chunk.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert.deepEqual(boss?.slice(1),['8290','3','Страж моста']);
assert(game.includes('only fragments gate the exit'));
assert(game.includes('sw.on=true;'));
console.log('PASS MAP04 source parity: 18 terrain solids, 90 coins, six enemies, three levers, guardian HP3 and boss-gated exit');
