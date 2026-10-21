/* Guard against libnx's SDMC auto-mount being treated as a failure.
 * Confirm both existing and newly mounted SDMC devices are supported and
 * only a registration we created ourselves is unmounted on exit.
 */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const main=fs.readFileSync(path.join(__dirname,
  '../switch_graphical/source/main.c'),'utf8');
assert(main.includes('fsdevGetDeviceFileSystem("sdmc")!=NULL'),
       'check existing libnx SDMC mount before any second mount');
assert(main.includes('already_mounted || R_SUCCEEDED(fsdevMountSdmc())'),
       'reuse default mount while retaining fallback explicit mount');
assert(main.includes('if(!already_mounted)sd_mounted=1;'),
       'do not own libnx auto-mounted filesystem');
assert(main.includes('if(sd_mounted)fsdevUnmountDevice("sdmc");'),
       'only unmount own registration on game exit');
const startup=main.indexOf('const int already_mounted=');
const firstLoad=main.indexOf('if(save_ready){');
assert(startup>=0 && firstLoad>startup,
       'SDMC setup must happen before RKC2/RKS1/RKL3 restore');
assert(main.includes('if(mkdir("sdmc:/switch/RIKITU",0777)==0 || errno==EEXIST)'));
console.log('PASS Stage95 Switch SDMC: existing libnx mount reused, save path initialized');
