/* MAP16 Heart Chamber exact source parity with the original browser game.js. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const root=path.join(__dirname,'..');
const script=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_heart_data.h'),'utf8');
const draw=fs.readFileSync(path.join(root,'native/rikitu_draw.h'),'utf8');
const start=script.indexOf("{name:'Зал Сердца'");
const end=script.indexOf('\n  ].map(d=>',start);
assert(start>=0&&end>start);
const src=script.slice(start,end);
function arr(k,next){
 const m=src.match(new RegExp(k+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'browser array '+k);return JSON.parse(m[1]);
}
function native(name,n){
 const m=header.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'native array '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(v=>{
  const nums=v[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
  assert.equal(nums.length,n,name);
  return nums;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','conveyors');
const belts=arr('conveyors','vents');
const frags=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const launcherX=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(z=>+z[1]);
const enemies=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((v,i)=>[+v[1],428,33,28,+v[2],+v[3],i%2?1:-1,0]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
 ...platforms.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
 for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of platforms)
 for(const t of [.25,.5,.75])coins.push([Math.round(x+w*t),y-46]);
let i=0;
while(coins.length<90){
 const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
 if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))coins.push([x,425]);
 if(++i>5000)throw Error('coin placement overflow');
}
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(belts.length,3);
assert.equal(enemies.length,6);
assert.equal(launcherX.length,5);
assert.deepEqual(native('rikitu_heart_solids',4),solids);
assert.deepEqual(native('rikitu_heart_fragments',2),frags);
assert.deepEqual(native('rikitu_heart_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_heart_enemies',8),enemies);
assert.deepEqual(native('rikitu_heart_belts',3),
 belts.map(([x,y,w,speed])=>{assert.equal(y,430);return [x,w,speed];}));
const launches=header.match(/static const RikituLauncher rikitu_heart_launchers\[5\]=\{([\s\S]*?)\};/);
assert(launches);
assert.deepEqual([...launches[1].matchAll(/\{(RIKITU_LAUNCH_(?:STEAM|CLOCK)),\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(m=>[m[1],+m[2],+m[3]]),
 [...launcherX.slice(0,3).map(x=>['RIKITU_LAUNCH_STEAM',x,456]),
  ...launcherX.slice(3).map(x=>['RIKITU_LAUNCH_CLOCK',x,456])]);
assert.match(draw,/RIKITU_MAX_LAUNCHERS\s*=\s*5/);
assert.deepEqual(header.match(/rikitu_heart_checkpoints\[3\]=\{([^}]+)\}/)[1]
 .split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),11750);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),11560);
const boss=src.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert.deepEqual(boss?.slice(1),['10950','3','Хранитель минут']);
console.log('PASS MAP16 source parity: 18 solids, 90 coins, 3 steam, 2 clock, 3 belts, boss');
