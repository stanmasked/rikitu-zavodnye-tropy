/* Compare C99 conveyors and launchers to the actual game.js surface block. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const {execFileSync}=require('node:child_process');
const binary=process.argv[2];
if(!binary)throw Error('usage: node tests/test-surfaces-parity.cjs <probe-executable>');
const game=fs.readFileSync(path.join(__dirname,'..','game.js'),'utf8');
const begin=game.indexOf('    // Gentle moving floor belts');
const end=game.indexOf('    // One-touch bridge levers',begin);
assert(begin>=0&&end>begin,'surface block changed: update parity test');
const script=game.slice(begin,end);
for(const marker of ['level.conveyors','level.vents','level.clockPads',
 'level.gusts','level.crystals','level.mushrooms']){
  assert(script.includes(marker),'surface type removed from source: '+marker);
}
const nativeKind={steam:1,clock:2,gust:3,crystal:4,mushroom:5};
const runBrowser=new Function('p','level','dt','clamp','announce',script);
let seed=0x71924680;
const rnd=()=>{seed=(Math.imul(1664525,seed)+1013904223)>>>0;return seed/4294967296;};
const ri=(lo,hi)=>lo+Math.floor(rnd()*(hi-lo+1));
const cases=[];
const scenario=(name,x,y,grounded,belts,launchers)=>({name,x,y,grounded,vy:0,dt:1/60,width:1000,belts,launchers});
const kinds=['steam','clock','gust','crystal','mushroom'];
for(const kind of kinds) cases.push(scenario('single '+kind,140,57,true,[],[{kind,x:155,y:100}]));
cases.push(scenario('precedence vs reverse array',140,57,true,[],kinds.slice().reverse().map(kind=>({kind,x:155,y:100}))));
cases.push(scenario('belt into steam',50,57,true,[{x:60,w:55,speed:600}],[{kind:'steam',x:90,y:100}]));
cases.push(scenario('not grounded',50,57,false,[{x:60,w:55,speed:600}],[{kind:'steam',x:90,y:100}]));
cases.push(scenario('no mushroom at exactly 7',140,57,true,[],[{kind:'mushroom',x:155,y:107}]));
cases.push(scenario('mushroom inside 7',140,57,true,[],[{kind:'mushroom',x:155,y:106}]));
cases.push(scenario('near right boundary',966,57,true,[{x:950,w:100,speed:900}],[]));
cases.push(scenario('near left boundary',1,57,true,[{x:0,w:100,speed:-900}],[]));
for(let i=0;i<400;i++){
  const x=ri(0,967);
  const grounded=ri(0,9)<7;
  const y=ri(0,4)===0?ri(32,100):57;
  const belts=Array.from({length:ri(0,3)},()=>({
    x:x+ri(-60,60),w:ri(10,120),speed:ri(-850,850)
  }));
  const launchers=Array.from({length:ri(0,6)},()=>({
    kind:kinds[ri(0,4)],x:x+ri(-70,70),y:ri(0,1)?100:ri(70,130)
  }));
  cases.push(scenario('seeded '+i,x,y,grounded,belts,launchers));
}
let input='',expected=[];
for(const c of cases){
  const p={x:c.x,y:c.y,w:33,h:43,vx:0,vy:c.vy,grounded:c.grounded};
  const level={width:c.width,conveyors:c.belts.map(b=>({...b})),
    vents:[],clockPads:[],gusts:[],crystals:[],mushrooms:[]};
  for(const l of c.launchers){
    level[({steam:'vents',clock:'clockPads',gust:'gusts',
      crystal:'crystals',mushroom:'mushrooms'})[l.kind]].push({x:l.x,y:l.y});
  }
  runBrowser(p,level,c.dt,(n,lo,hi)=>Math.min(hi,Math.max(lo,n)),()=>{});
  expected.push({name:c.name,x:p.x,vy:p.vy,grounded:p.grounded,launched:c.grounded&&!p.grounded});
  input+='S '+[c.x,c.y,Number(c.grounded),c.vy,c.dt,c.width].join(' ')+'\n';
  for(const b of c.belts) input+='B '+[b.x,b.w,b.speed].join(' ')+'\n';
  for(const l of c.launchers) input+='L '+[nativeKind[l.kind],l.x,l.y].join(' ')+'\n';
  input+='E\n';
}
const output=execFileSync(binary,{input,encoding:'utf8',maxBuffer:1024*1024}).trim().split(/\r?\n/);
assert.equal(output.length,cases.length,'C99 probe line count');
for(let i=0;i<output.length;i++){
  const line=output[i].trim().split(/\s+/).map(Number);
  assert.equal(line.length,4,'probe output schema '+i);
  const js=expected[i], [x,vy,grounded,kind]=line;
  assert(Math.abs(x-js.x)<0.015,js.name+' horizontal: C99='+x+' JS='+js.x);
  assert(Math.abs(vy-js.vy)<0.015,js.name+' vertical: C99='+vy+' JS='+js.vy);
  assert.equal(grounded,Number(js.grounded),js.name+' grounded');
  assert.equal(kind!==0,js.launched,js.name+' launcher fired');
}
console.log('PASS real game.js surfaces vs C99: '+cases.length+' deterministic cases');
