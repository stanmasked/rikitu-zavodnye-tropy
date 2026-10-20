/* Compare all six native Flower Trail enemies against live browser source. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.join(__dirname,'..');
const game=fs.readFileSync(path.join(root,'game.js'),'utf8');
const native=fs.readFileSync(path.join(root,'native/rikitu_flower_enemies_data.h'),'utf8');
const begin=game.indexOf("{name:'Цветочная тропа'");
const end=game.indexOf("{name:'Мельница у ручья'",begin);
assert(begin>=0&&end>begin);
const source=game.slice(begin,end);
const list=source.match(/enemies:\s*(\[[\s\S]*?\]),\s*sections:/);
assert(list,'Flower Trail enemy array not found in game.js');
const browser=[...list[1].matchAll(/\{\s*x:(\d+),a:(\d+),b:(\d+)\}/g)]
  .map((m,i)=>[Number(m[1]),456-28,33,28,Number(m[2]),Number(m[3]),
              i%2?1:-1,0]);
const arr=native.match(/static const RikituEnemy rikitu_flower_enemy_spawns\[6\]=\{([\s\S]*?)\};/);
assert(arr,'C99 spawn data missing');
const c=[...arr[1].matchAll(/\{([^{}]+)\}/g)]
  .map(m=>m[1].split(',').map(x=>Number(x.trim().replace(/f$/,''))));
assert.equal(browser.length,6);
assert.deepEqual(c,browser,'Native patrol positions/bounds/speeds/directions must match browser game.js');
const gameCore=fs.readFileSync(path.join(root,'game-core.js'),'utf8');
assert(gameCore.includes('invincibility: 1.6'),'Expected 1.6s browser invulnerability');
const lines=game.slice(game.indexOf('function damage(){'),game.indexOf('function jumpPressed()'));
assert(lines.includes('p.inv=1.4'),'Expected 1.4s respawn invulnerability');
assert(lines.includes("p.x=p.spawnX"),'Expected checkpoint respawn');
assert(game.includes("e.x+=e.dir*43*dt"),'Expected 43 units/s patrol speed');
assert(game.includes("p.vy=-360"),'Expected stomp bounce');
console.log('PASS all six Flower Trail patrols, y/h, alternating directions and browser combat timing constants');
