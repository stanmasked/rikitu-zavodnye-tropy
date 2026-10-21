/* Stage93 guard: the shipping NRO and real NeXium QA agree on 16 maps.
 * Static contract only; screenshot verification runs separately in emulator.
 */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const src=p=>fs.readFileSync(path.join(__dirname,'..',p),'utf8');
const main=src('switch_graphical/source/main.c');
const draw=src('native/rikitu_draw.c');
const runner=src('tests/run_nexium_graphical.sh');
const workflow=src('.github/workflows/switch-graphics-emulator.yml');
const check=src('tests/check_emulator_map_cycle.py');
const match=main.match(/static void \(\*const stage_init\[16\]\)\(RikituScene \*\)=\{([\s\S]*?)\};/);
assert(match,'missing 16-map init registry in actual Switch NRO');
const init=match[1].match(/rikitu_\w+_init/g)||[];
assert.equal(init.length,16,'actual Switch must register exactly 16 scene inits');
assert.equal(new Set(init).size,16,'each chapter initializer must be distinct');
for(let i=1;i<=16;i++){
  assert(draw.includes('RIKITU '+String(i).padStart(2,'0')+' '),
         'HUD must label map '+i);
}
assert(main.includes('if(pressed & HidNpadButton_Y){'));
assert(main.includes('scene.level_id==16?1:scene.level_id+1'));
assert(main.includes('campaign_debug=1;'));
assert(workflow.includes('export RIKITU_MAP_CYCLE_QA=1'));
assert(runner.includes('xdotool keydown --window "$window" z'));
assert(runner.includes('python3 tests/check_emulator_map_cycle.py "$out"'));
assert(check.includes('map-wrap-01.png'));
assert(check.includes('int(246 * sx)'));
assert(check.includes('int(510 * sx)'));
assert(check.includes('for stage in range(1, 17)'));
console.log('PASS Stage93 16-map live NeXium selector and screenshot QA wiring');
