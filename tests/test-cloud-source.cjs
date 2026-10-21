/* MAP10 Cloud Dock source-parity audit against original browser game.js. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const h=fs.readFileSync(path.join(root,'native/rikitu_cloud_data.h'),'utf8');
const start=game.indexOf("{name:'Облачная пристань'");
const end=game.indexOf("{name:'Кристальные пещеры'",start);
assert(start>=0 && end>start);
const src=game.slice(start,end);
function arr(name,next){
 const m=src.match(new RegExp(name+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser '+name);return JSON.parse(m[1]);
}
function native(name,count){
 const m=h.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,'Missing C99 array '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(y=>{
  const v=y[1].split(',').map(z=>Number(z.trim().replace(/f$/,'')));
  assert.equal(v.length,count,name);return v;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','gusts');
const conveyors=arr('conveyors','fragments');
const fragments=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const gusts=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>[+m[1],456]);
const enemies=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
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
 if(++i>5000)throw Error('coin generation exceeded');
}
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(gusts.length,4);
assert.equal(conveyors.length,2);
assert.equal(enemies.length,6);
assert.deepEqual(native('rikitu_cloud_solids',4),solids);
assert.deepEqual(native('rikitu_cloud_fragments',2),fragments);
assert.deepEqual(native('rikitu_cloud_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_cloud_enemies',8),enemies);
assert.deepEqual(native('rikitu_cloud_belts',3),
 conveyors.map(([x,y,w,speed])=>{assert.equal(y,430);return [x,w,speed];}));
const cppGust=h.match(/static const RikituLauncher rikitu_cloud_gusts\[4\]=\{([\s\S]*?)\};/);
assert(cppGust);
assert.deepEqual([...cppGust[1].matchAll(/\{RIKITU_LAUNCH_GUST,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(m=>[+m[1],+m[2]]),gusts);
const nativeCps=h.match(/static const float rikitu_cloud_checkpoints\[3\]=\{([^}]+)\};/);
assert(nativeCps);
assert.deepEqual(nativeCps[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),10320);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),10140);
const boss=src.match(/boss:\{x:(\d+),hp:(\d+),name:'([^']+)'\}/);
assert.deepEqual(boss?.slice(1),['9650','3','Ветряной автомат']);
console.log('PASS MAP10 Cloud Dock: 18 solids, 90 coins, 4 gusts, 2 belts, 6 patrols, 3-hit boss');
