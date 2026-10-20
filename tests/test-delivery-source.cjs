/* MAP06 browser source is authoritative: validate all geometry and objects. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const h=fs.readFileSync(path.join(root,'native/rikitu_delivery_data.h'),'utf8');
const begin=game.indexOf("{name:'Подземная доставка'");
const end=game.indexOf("{name:'Часовая площадь'",begin);
assert(begin>=0 && end>begin);
const src=game.slice(begin,end);
function arr(key,next){
 const m=src.match(new RegExp(key+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser '+key);
 return JSON.parse(m[1]);
}
function native(name,arity){
 const m=h.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'Missing native '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(z=>{
   const parts=z[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
   assert.equal(parts.length,arity,name);
   return parts;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','conveyors');
const belts=arr('conveyors','vents');
const fragments=arr('fragments','checkpoints');
const cps=arr('checkpoints','enemies');
const vents=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>[+m[1],456]);
const enemies=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
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
 if(++i>5000)throw Error('coin synthesis failed');
}
const ncp=h.match(/static const float rikitu_delivery_checkpoints\[3\]=\{([^}]+)\};/);
assert(ncp);
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(enemies.length,6);
assert.equal(vents.length,3);
assert.equal(belts.length,3);
assert.deepEqual(native('rikitu_delivery_solids',4),solids);
assert.deepEqual(native('rikitu_delivery_fragments',2),fragments);
assert.deepEqual(native('rikitu_delivery_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_delivery_enemies',8),enemies);
assert.deepEqual(native('rikitu_delivery_belts',3),belts.map(([x,y,w,speed])=>{
 assert(y===430||y===431);return [x,w,speed];
}));
const nvents=h.match(/static const RikituLauncher rikitu_delivery_vents\[3\]=\{([\s\S]*?)\};/);
assert(nvents);
const rows=[...nvents[1].matchAll(/\{RIKITU_LAUNCH_STEAM,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(m=>[+m[1],+m[2]]);
assert.deepEqual(rows,vents);
assert.deepEqual(ncp[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),cps);
assert.equal(Number(src.match(/width:(\d+)/)[1]),9910);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),9730);
const boss=src.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert.deepEqual(boss?.slice(1),['9210','3','Пыльный бурильщик']);
console.log('PASS MAP06 browser parity: 18 solids, 90 coins, 3 belts, 3 steam vents, 6 patrols and boss');
