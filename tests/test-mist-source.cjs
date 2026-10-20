/* MAP09 gust source parity: all geometry, pickups and enemy patrols. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const root=path.join(__dirname,'..');
const script=fs.readFileSync(path.join(root,'game.js'),'utf8');
const h=fs.readFileSync(path.join(root,'native/rikitu_mist_data.h'),'utf8');
const a=script.indexOf("{name:'Туманный перевал'");
const b=script.indexOf("{name:'Облачная пристань'",a);
assert(a>=0&&b>a);
const src=script.slice(a,b);
function arr(key,next){
 const m=src.match(new RegExp(key+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser data: '+key);
 return JSON.parse(m[1]);
}
function native(name,n){
 const m=h.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\{([\\s\\S]*?)\\};'));
 assert(m,name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(row=>{
  const values=row[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
  assert.equal(values.length,n,name);
  return values;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','gusts');
assert(!src.includes('conveyors:'));

const fragments=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const gusts=[...src.matchAll(/\[(\d+),GROUND\]/g)].map(m=>[+m[1],456]);
const enemies=[...src.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
 .map((x,i)=>[+x[1],428,33,28,+x[2],+x[3],i%2?1:-1,0]);
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
 if(++i>5000)throw Error('coin generation failed');
}
assert.equal(solids.length,18);
assert.equal(gusts.length,3);
assert.equal(enemies.length,6);
assert.deepEqual(native('rikitu_mist_solids',4),solids);
assert.deepEqual(native('rikitu_mist_fragments',2),fragments);
assert.deepEqual(native('rikitu_mist_coins',2),coins.slice(0,90));
assert.deepEqual(native('rikitu_mist_enemies',8),enemies);

const pads=h.match(/static const RikituLauncher rikitu_mist_gusts\[3\]=\{([\s\S]*?)\};/);
assert(pads);
assert.deepEqual([...pads[1].matchAll(/\{RIKITU_LAUNCH_GUST,\s*(\d+(?:\.\d+)?)f,\s*(\d+(?:\.\d+)?)f\}/g)]
 .map(x=>[+x[1],+x[2]]),gusts);
const ncps=h.match(/static const float rikitu_mist_checkpoints\[3\]=\{([^}]+)\};/);
assert(ncps);
assert.deepEqual(ncps[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(src.match(/width:(\d+)/)[1]),9520);
assert.equal(Number(src.match(/gate:(\d+)/)[1]),9330);
assert(src.includes('boss:null'));
console.log('PASS MAP09 Misty Pass source parity: 18 solids, 90 coins, 6 patrols, 3 gusts, no conveyors');
