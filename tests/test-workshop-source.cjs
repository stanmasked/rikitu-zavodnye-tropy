/* Compare EVERY MAP05 terrain/pickup/belt/patrol with authoritative browser game.js. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const header=fs.readFileSync(path.join(root,'native/rikitu_workshop_data.h'),'utf8');
const a=game.indexOf("{name:'Улица мастерских'");
const b=game.indexOf("{name:'Подземная доставка'",a);
assert(a>=0&&b>a);
const source=game.slice(a,b);
function arr(key,next){
 const m=source.match(new RegExp(key+':\\s*(\\[[\\s\\S]*?\\]),\\s*'+next+':'));
 assert(m,'Missing browser array '+key);return JSON.parse(m[1]);
}
function cdata(name,arity){
 const m=header.match(new RegExp('static const \\w+ '+name+'\\[\\d+\\]=\\s*\\{([\\s\\S]*?)\\};'));
 assert(m,'Missing native array '+name);
 return [...m[1].matchAll(/\{([^{}]+)\}/g)].map(q=>{
  const values=q[1].split(',').map(z=>Number(z.trim().replace(/f$/,'')));
  assert.equal(values.length,arity,name);return values;
 });
}
const ground=arr('ground','platforms');
const platforms=arr('platforms','conveyors');
const conveyors=arr('conveyors','fragments');
const fragments=arr('fragments','checkpoints');
const checkpoints=arr('checkpoints','enemies');
const enemies=[...source.matchAll(/\{x:(\d+),a:(\d+),b:(\d+)\}/g)]
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
 if(++i>5000)throw Error('Cannot generate 90 source coins');
}
const belts=conveyors.map(([x,y,w,speed])=>{assert.equal(y,430);return [x,w,speed];});
const cps=header.match(/static const float rikitu_workshop_checkpoints\[3\]=\{([^}]+)\};/);
assert(cps);
assert.equal(ground.length,5);
assert.equal(platforms.length,13);
assert.equal(coins.length>=90,true);
assert.equal(enemies.length,6);
assert.equal(conveyors.length,3);
assert.deepEqual(cdata('rikitu_workshop_solids',4),solids);
assert.deepEqual(cdata('rikitu_workshop_fragments',2),fragments);
assert.deepEqual(cdata('rikitu_workshop_coins',2),coins.slice(0,90));
assert.deepEqual(cdata('rikitu_workshop_enemies',8),enemies);
assert.deepEqual(cdata('rikitu_workshop_belts',3),belts);
assert.deepEqual(cps[1].split(',').map(v=>Number(v.trim().replace(/f$/,''))),checkpoints);
assert.equal(Number(source.match(/width:(\d+)/)[1]),9140);
assert.equal(Number(source.match(/gate:(\d+)/)[1]),8950);
assert(source.includes('boss:null'));
console.log('PASS MAP05 source parity: 18 solids, 90 coins, 6 patrols, 3 conveyors, 3 fragments and 3 checkpoints');
