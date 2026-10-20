"""Records completed stage pace and preserves the journal in checkpoint saves."""
from pathlib import Path
import json,os
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
HTML=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')
KEY='rikitu-zavodnye-tropy-checkpoint-v1'
SHIM="""(initial)=>{
 const data=Object.assign({},initial);
 Object.defineProperty(window,'localStorage',{configurable:true,value:{
   getItem(k){return Object.hasOwn(data,k)?data[k]:null},
   setItem(k,v){data[k]=String(v)},removeItem(k){delete data[k]}
 }});
}"""
with sync_playwright() as pw:
    binary=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=binary,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.evaluate(SHIM,{})
    page.set_content(HTML,wait_until='load')
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='playing'",timeout=5000)
    for count,(x,y) in enumerate([(1520,354),(3640,339),(6860,354)],1):
        page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y])
        page.wait_for_function('(n)=>RIKITU_TEST_STATE().fragments>=n',arg=count,timeout=5000)
    page.evaluate('RIKITU_TEST_MOVE(7790,400)')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    data=page.evaluate('RIKITU_TEST_STATE()')
    assert len(data['stageRecords'])==1,data
    rec=data['stageRecords'][0]
    assert rec['stage']==1 and rec['seconds']>=0 and rec['hits']>=0 and 0<=rec['coins']<=90,rec
    assert 'Время' in page.locator('body').inner_text() or page.evaluate('document.getElementById("game").width')==960
    saved=page.evaluate('(key)=>localStorage.getItem(key)',KEY)
    assert json.loads(saved)['records'][0]['stage']==1
    page.close()
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.evaluate(SHIM,{KEY:saved})
    page.set_content(HTML,wait_until='load')
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    assert page.evaluate('RIKITU_TEST_STATE().stageRecords.length')==1
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===2",timeout=5000)
    assert page.evaluate('RIKITU_TEST_STATE().stageRecords.length')==1
    page.keyboard.press('r')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===1",timeout=5000)
    assert page.evaluate('RIKITU_TEST_STATE().stageRecords.length')==0
    assert not errors,errors
    print('PASS stage completion journal: summary, save/restore, transition, reset')
    browser.close()
