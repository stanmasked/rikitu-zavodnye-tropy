/* MAP12 Silent Mine browser to C99 coordinate and mechanic parity. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const srcfile=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_mine_data.h'),'utf8');
const start=srcfile.indexOf("{name:'Шахта молчания'");
const end=srcfile.indexOf("{name:'Замковые сады'",start);
assert(start>=0&&end>start);
const src=srcfile.slice(start,end);
function arr(k,next){
 const m=src.match(new RegExp(k+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'browser array '+k);return JSON.parse(m[1]);
}
function native(name,n){
 const m=header.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'C99 array '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(z=>{
  const a=z[1].split(',').map(v=>Number(v.trim().replace(/f$/,'')));
  assert.equal(a.length,n,name);return a;
 });
}
const ground=arr('ground','platforms');
const plats=arr('platforms','conveyors');
const belts=arr('conveyors','vents');
const frags=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const pts=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>+m[1]);
const vents=pts.slice(0,3).map(x=>[x,456]);
const levers=pts.slice(3).map(x=>[x,456,0]);
const enemy=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
 ...plats.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
 for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of plats)
 for(const t of [.25,.5,.75])coins.push([Math.round(x+w*t),y-46]);
let i=0;
while(coins.length<90){
 const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
 if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))coins.push([x,425]);
 if(++i>5000)throw Error('coins overflow');
}
assert.equal(ground.length,5);
assert.equal(plats.length,13);
assert.equal(belts.length,3);
assert.equal(vents.length,3);
assert.equal(levers.length,3);
assert.equal(enemy.length,6);
assert.deepEqual(native('rikitu_mine_solids',4),solids);
assert.deepEqual(native('rikitu_mine_fragments',2),frags);
assert.deepEqual(native('rikitu_mine_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_mine_enemies',8),enemy);
assert.deepEqual(native('rikitu_mine_belts',3),
 belts.map(([x,y,w,speed])=>{assert.equal(y,430);return [x,w,speed];}));
assert.deepEqual(native('rikitu_mine_levers',3),levers);
const vs=header.match(/static const RikituLauncher rikitu_mine_vents\[3\]=\{([\s\S]*?)\};/);
assert(vs);
assert.deepEqual([...vs[1].matchAll(/\{RIKITU_LAUNCH_STEAM,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(m=>[+m[1],+m[2]]),vents);
const cp=header.match(/static const float rikitu_mine_checkpoints\[3\]=\{([^}]+)\};/);
assert(cp);
assert.deepEqual(cp[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),10700);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),10530);
const boss=src.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert.deepEqual(boss?.slice(1),['9930','3','Буровой караул']);
console.log('PASS MAP12 source parity: 18 solids, 90 coins, 3 belts, 3 steam vents, 3 levers, boss');
