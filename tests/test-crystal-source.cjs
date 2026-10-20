/* MAP11 Crystal Caves: compare exact browser coordinates with C99 source data. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const srcfile=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_crystal_data.h'),'utf8');
const start=srcfile.indexOf("{name:'Кристальные пещеры'");
const end=srcfile.indexOf("{name:'Шахта молчания'",start);
assert(start>=0&&end>start);
const src=srcfile.slice(start,end);
function arr(name,next){
 const m=src.match(new RegExp(name+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser '+name);return JSON.parse(m[1]);
}
function native(name,n){
 const m=header.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'Missing native '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(z=>{
  const row=z[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
  assert.equal(row.length,n,name);return row;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','crystals');
const fragments=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const crystals=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>[+m[1],456]);
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
 if(++i>5000)throw Error('coin generation overflow');
}
assert.equal(solids.length,18);
assert.equal(crystals.length,3);
assert.equal(enemies.length,6);
assert(!src.includes('conveyors:'));
assert.deepEqual(native('rikitu_crystal_solids',4),solids);
assert.deepEqual(native('rikitu_crystal_fragments',2),fragments);
assert.deepEqual(native('rikitu_crystal_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_crystal_enemies',8),enemies);
const springs=header.match(/static const RikituLauncher rikitu_crystal_springs\[3\]=\{([\s\S]*?)\};/);
assert(springs);
assert.deepEqual([...springs[1].matchAll(/\{RIKITU_LAUNCH_CRYSTAL,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(m=>[+m[1],+m[2]]),crystals);
const cp=header.match(/static const float rikitu_crystal_checkpoints\[3\]=\{([^}]+)\};/);
assert(cp);
assert.deepEqual(cp[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),9820);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),9620);
assert(src.includes('boss:null'));
console.log('PASS MAP11 original source parity: 18 solids, 90 coins, 3 crystal springs, 6 patrols');
