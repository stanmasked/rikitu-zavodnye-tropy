/* Validate complete native Flower Trail geometry against real game.js source.
 * No duplicate JSON/JS copy of the map is trusted as the expected result.
 */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const js=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_flower_data.h'),'utf8');
const start=js.indexOf("{name:'Цветочная тропа'");
const end=js.indexOf("{name:'Мельница у ручья'",start);
assert(start>=0&&end>start,'Flower Trail source cannot be located');
const source=js.slice(start,end);
function sourceArray(key,next){
  const re=new RegExp(key+':(\\[[\\s\\S]*?\\]),\\s*'+next+':');
  const match=source.match(re);
  assert(match,'Missing source array: '+key);
  return JSON.parse(match[1]);
}
function nativeArray(name,arity){
  const regex=new RegExp('static const \\w+ '+name+'\\[\\d+\\] = \\{([\\s\\S]*?)\\};');
  const match=header.match(regex);
  assert(match,'Missing generated C data array '+name);
  const rows=[...match[1].matchAll(/\{([^{}]+)\}/g)];
  assert(rows.length>0,'Empty C array '+name);
  return rows.map(row=>{
    const values=row[1].split(',').map(x=>Number(x.trim().replace(/f$/i,'')));
    assert.equal(values.length,arity,name+' coordinate arity');
    assert(values.every(Number.isFinite),name+' nonnumeric coordinate');
    return values;
  });
}
const ground=sourceArray('ground','platforms');
const platforms=sourceArray('platforms','fragments');
const fragments=sourceArray('fragments','checkpoints');
const checkpoints=sourceArray('checkpoints','enemies');
const world=Number(source.match(/width:(\d+)/)[1]);
const gate=Number(source.match(/gate:(\d+)/)[1]);
const expectedSolids=[
  ...ground.map(([a,b])=>[a,456,b-a,540-456+180]),
  ...platforms.map(([x,y,w])=>[x,y,w,16])
];
const expectedCoins=[];
for(const [a,b] of ground)
  for(let x=Math.max(160,a+135);x<b-80;x+=145)
    expectedCoins.push([x,393]);
for(const [x,y,w] of platforms)
  for(const ratio of [0.25,0.5,0.75])
    expectedCoins.push([Math.round(x+w*ratio),y-46]);
let i=0;
while(expectedCoins.length<90){
  const [a,b]=ground[i%ground.length];
  const x=a+65+Math.floor(i/ground.length)*47;
  if(x<b-30&&!expectedCoins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))
    expectedCoins.push([x,425]);
  if(++i>5000)throw Error('coin placement diverged');
}
assert.equal(world,7900);
assert.equal(gate,7770);
assert.equal(fragments.length,3);
assert.equal(checkpoints.length,3);
assert.equal(expectedSolids.length,19);
assert.equal(expectedCoins.length,90);
assert.deepEqual(nativeArray('rikitu_flower_solids',4),expectedSolids,'all solid rectangles');
assert.deepEqual(nativeArray('rikitu_flower_fragments',2),fragments,'all fragments');
assert.deepEqual(nativeArray('rikitu_flower_coins',2),expectedCoins,'90 coins');
const cp=header.match(/static const float rikitu_flower_checkpoints\[3\] = \{([^}]+)\};/);
assert(cp,'Missing checkpoints');
assert.deepEqual(cp[1].split(',').map(s=>Number(s.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(header.match(/#define RIKITU_FLOWER_WORLD_WIDTH (\d+)\.0f/)[1]),world);
assert.equal(Number(header.match(/#define RIKITU_FLOWER_GATE (\d+)\.0f/)[1]),gate);
console.log('PASS exact game.js parity: 5 ground spans, 14 platforms, 3 fragments, 3 checkpoints, 90 coins, gate 7770, width 7900');
