/* MAP03 Mushroom Grove data must match the current game.js, not a fixture. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_grove_data.h'),'utf8');
const start=game.indexOf("{name:'Грибная роща'");
const end=game.indexOf("{name:'Мост шестерён'",start);
assert(start>=0&&end>start);
const source=game.slice(start,end);
function extractArray(k,next){
  const m=source.match(new RegExp(k+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
  assert(m,'Missing source array '+k);return JSON.parse(m[1]);
}
function rows(name,arity){
  const m=header.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\] = \\{([\\s\\S]*?)\\};'));
  assert(m,'Missing C99 array '+name);
  return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(z=>{
    const values=z[1].split(',').map(x=>Number(x.trim().replace(/f$/,'')));
    assert.equal(values.length,arity,name);
    return values;
  });
}
const ground=extractArray('ground','platforms');
const platforms=extractArray('platforms','mushrooms');
const fragments=extractArray('fragments','checkpoints');
const checkpoints=extractArray('checkpoints','enemies');
const enemies=[...source.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
  .map((m,i)=>[+m[1],428,33,28,+m[2],+m[3],i%2?1:-1,0]);
const mushrooms=[...source.matchAll(/\[(\d+),GROUND\]/g)].map(m=>+m[1]);
const solids=[...ground.map(([a,b])=>[a,456,b-a,264]),
              ...platforms.map(([x,y,w])=>[x,y,w,16])];
const coins=[];
for(const [a,b] of ground)
  for(let x=Math.max(160,a+135);x<b-80;x+=145)coins.push([x,393]);
for(const [x,y,w] of platforms)
  for(const t of [0.25,0.50,0.75])coins.push([Math.round(x+w*t),y-46]);
let i=0;
while(coins.length<90){
  const [a,b]=ground[i%ground.length],x=a+65+Math.floor(i/ground.length)*47;
  if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))
    coins.push([x,425]);
  if(++i>5000)throw Error('coin generation diverged');
}
assert.equal(Number(source.match(/width:(\d+)/)[1]),8270);
assert.equal(Number(source.match(/gate:(\d+)/)[1]),8140);
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(enemies.length,6);
assert.deepEqual(mushrooms,[940,2670,5660]);
assert.deepEqual(rows('rikitu_grove_solids',4),solids);
assert.deepEqual(rows('rikitu_grove_fragments',2),fragments);
assert.deepEqual(rows('rikitu_grove_coins',2),coins);
assert.deepEqual(rows('rikitu_grove_enemies',8),enemies);
const cp=header.match(/static const float rikitu_grove_checkpoints\[3\]=\{([^}]+)\};/);
assert(cp);
assert.deepEqual(cp[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))),checkpoints);
const lc=header.match(/static const RikituLauncher rikitu_grove_mushrooms\[3\] = \{([\s\S]*?)\};/);
assert(lc);
const launches=[...lc[1].matchAll(/\{RIKITU_LAUNCH_MUSHROOM, ([\d.]+)f, ([\d.]+)f\}/g)]
  .map(x=>[Number(x[1]),Number(x[2])]);
assert.deepEqual(launches,mushrooms.map(x=>[x,456]));
console.log('PASS MAP03 parity: 8270-unit world, 18 terrain pieces, 90 coins, 3 fragments/checkpoints, 6 enemies, 3 mushroom springs');
