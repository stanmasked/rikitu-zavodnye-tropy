/* MAP15 Clock Gallery: authoritative browser-to-C99 geometry audit. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const h=fs.readFileSync(path.join(root,'native/rikitu_gallery_data.h'),'utf8');
const start=game.indexOf("{name:'Галерея часов'");
const end=game.indexOf("{name:'",start+10);
assert(start>=0&&end>start);
const src=game.slice(start,end);
function arr(name,next){
 const m=src.match(new RegExp(name+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser '+name);return JSON.parse(m[1]);
}
function native(name,n){
 const m=h.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(row=>{
  const a=row[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
  assert.equal(a.length,n,name);return a;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','clockPads');
const fragments=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const raw=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>+m[1]);
const clockPads=raw.slice(0,3).map(x=>[x,456]);
const levers=raw.slice(3).map(x=>[x,456,0]);
const enemies=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
 ...platforms.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
 for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of platforms)
 for(const t of [.25,.5,.75])coins.push([Math.round(x+w*t),y-46]);
let i=0;while(coins.length<90){
 const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
 if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))coins.push([x,425]);
 if(++i>5000)throw Error('Cannot place 90 coins');
}
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(clockPads.length,3);
assert.equal(levers.length,3);
assert.equal(enemies.length,6);
assert(!src.includes('conveyors:'));
assert.deepEqual(native('rikitu_gallery_solids',4),solids);
assert.deepEqual(native('rikitu_gallery_fragments',2),fragments);
assert.deepEqual(native('rikitu_gallery_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_gallery_enemies',8),enemies);
assert.deepEqual(native('rikitu_gallery_levers',3),levers);
const pads=h.match(/static const RikituLauncher rikitu_gallery_clock_pads\[3\]=\{([\s\S]*?)\};/);
assert(pads);
assert.deepEqual([...pads[1].matchAll(/\{RIKITU_LAUNCH_CLOCK,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(x=>[+x[1],+x[2]]),clockPads);
const cp=h.match(/static const float rikitu_gallery_checkpoints\[3\]=\{([^}]+)\};/);
assert(cp);
assert.deepEqual(cp[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),10300);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),10120);
assert(src.includes('boss:null'));
console.log('PASS MAP15 original browser source parity: 18 solids, 90 coins, 3 clock pads, 3 levers and 6 patrols');
