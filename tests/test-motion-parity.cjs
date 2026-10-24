/* Replays the actual browser movement source against C99 snapshots.
 * Covers simple floor and jump dynamics, not hazards or moving platforms.
 */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const {execFileSync}=require('node:child_process');
const core=require('../game-core.js');
const binary=process.argv[2];
if(!binary)throw Error('usage: node tests/test-motion-parity.cjs <probe-executable>');
const game=fs.readFileSync(path.join(__dirname,'..','game.js'),'utf8');
const start=game.indexOf("    const left=keys.has('ArrowLeft')");
const end=game.indexOf('    // Gentle moving floor belts',start);
assert(start!==-1&&end>start,'game.js movement segment was changed; update parity harness');
const segment=game.slice(start,end);
for(const guard of ['p.vx=','p.jumpBuffer=','p.coyote=','p.vy=','p.y+=','p.grounded=false','const solids='])
  assert(segment.includes(guard),'missing browser movement guard: '+guard);
const playerStep=new Function('p','keys','touch','gamepadX','gamepadRun','jumpHeld','previous','dt','level','d','C','clamp','sfx',
  'let previousJump=previous.value;const jumpPressed=()=>jumpHeld;\n'+segment+'\nprevious.value=previousJump;');
const dt=1/60;
const floor={x:0,y:100,w:1000,h:300};
const I={LEFT:1,RIGHT:2,JUMP:4,RUN:8};
const cases=[
  {name:'walk and reverse',p:{x:50,y:57,grounded:1},floor:true,actions:[...Array(75).fill(I.RIGHT),...Array(75).fill(I.LEFT)]},
  {name:'run plus both directions',p:{x:930,y:57,grounded:1},floor:true,actions:[...Array(50).fill(I.RIGHT|I.RUN),...Array(30).fill(I.LEFT|I.RIGHT),...Array(20).fill(I.LEFT|I.RUN)]},
  {name:'jump hold, release, land',p:{x:90,y:57,grounded:1},floor:true,actions:[...Array(32).fill(I.RIGHT|I.JUMP),...Array(70).fill(I.RIGHT),...Array(5).fill(I.JUMP),...Array(80).fill(0)]},
  {name:'buffered jump on descent',p:{x:40,y:47,vy:250},floor:true,actions:[...Array(8).fill(I.JUMP),...Array(65).fill(0)]},
  {name:'coyote jump',p:{x:120,y:57,coyote:0.06},floor:false,actions:[...Array(2).fill(0),I.JUMP,...Array(50).fill(0)]},
  {name:'no floor terminal velocity',p:{x:120,y:20},floor:false,actions:Array(120).fill(I.RIGHT)}
];
let stdin='',expected=[];
for(const scene of cases){
  const src=scene.p;
  const p={x:src.x,y:src.y,vx:src.vx||0,vy:src.vy||0,w:33,h:43,
    grounded:!!src.grounded,coyote:src.coyote||0,jumpBuffer:src.jumpBuffer||0,
    facing:src.facing||1};
  const previous={value:!!src.previousJump};
  stdin+='I '+[p.x,p.y,p.vx,p.vy,p.w,p.h,Number(p.grounded),p.coyote,p.jumpBuffer,Number(previous.value),p.facing].join(' ')+'\n';
  const level={width:1000,ground:scene.floor?[floor]:[],platforms:[]};
  for(const mask of scene.actions){
    const keys=new Set();
    if(mask&I.LEFT)keys.add('ArrowLeft');
    if(mask&I.RIGHT)keys.add('ArrowRight');
    if(mask&I.RUN)keys.add('ShiftLeft');
    playerStep(p,keys,{left:false,right:false,jump:false},0,false,!!(mask&I.JUMP),previous,dt,
               level,core.SETTINGS,core,(x,min,max)=>Math.min(max,Math.max(min,x)),()=>{});
    expected.push({scene:scene.name,p:{...p},previous:previous.value});
    stdin+='S '+mask+' '+Number(scene.floor)+'\n';
  }
}
const output=execFileSync(binary,{input:stdin,encoding:'utf8',maxBuffer:5*1024*1024});
const lines=output.trim().split(/\r?\n/);
assert.equal(lines.length,expected.length,'native output line count');
const epsilon={x:0.15,y:0.15,vx:0.03,vy:0.15,coyote:0.002,jumpBuffer:0.002};
for(let i=0;i<lines.length;i++){
  const values=lines[i].trim().split(/\s+/).map(Number);
  assert.equal(values.length,9,'case '+i+': malformed C output');
  const {p,previous,scene}=expected[i];
  const fields=['x','y','vx','vy','coyote','jumpBuffer'];
  for(let j=0;j<fields.length;j++){
    const name=fields[j],err=Math.abs(values[j]-p[name]);
    assert(Number.isFinite(values[j])&&err<=epsilon[name],
      scene+' frame '+i+' '+name+': native='+values[j]+' JS='+p[name]+' delta='+err);
  }
  assert.equal(values[6],Number(p.grounded),'grounded '+scene+' '+i);
  assert.equal(values[7],Number(previous),'previousJump '+scene+' '+i);
  assert.equal(values[8],p.facing,'facing '+scene+' '+i);
}
console.log('PASS browser actual movement segment versus C99: '+lines.length+' replayed frames across '+cases.length+' scenarios');
