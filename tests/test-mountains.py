"""World III opening: old-stage save, airlift, stage 10 boss and third Moki heart."""
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
def state(page):return page.evaluate('RIKITU_TEST_STATE()')
def move(page,x,y):page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y])
def collect(page,positions):
    for n,(x,y) in enumerate(positions,1):
        move(page,x,y)
        page.wait_for_function('(n)=>RIKITU_TEST_STATE().fragments>=n',arg=n,timeout=5000)
with sync_playwright() as p:
    exe=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').exists() else None)
    browser=p.chromium.launch(headless=True,executable_path=exe,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    save={'version':1,'levelIndex':7,'mode':'victory','wallet':260,'purchased':2,
          'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
          'switches':[],'checkpoint':7710,'hearts':5,'bossDown':True}
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==12 and all(x['coins']==90 and len(x['fragments'])==3 for x in levels)
    assert len(levels[8]['gusts'])==3 and len(levels[9]['gusts'])==4
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===9",timeout=5000)
    assert state(page)['wallet']==260 and state(page)['maxHearts']==5
    move(page,1250,413)
    page.wait_for_function('()=>RIKITU_TEST_STATE().playerY<390',timeout=5000)
    page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    collect(page,[(1730,350),(4720,350),(8160,351)])
    move(page,9345,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===10",timeout=5000)
    page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    collect(page,[(1890,350),(5200,347),(8730,349)])
    move(page,10160,400)
    page.wait_for_timeout(150)
    assert state(page)['mode']=='playing'
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for attempt in range(4):
            boss=state(page)
            move(page,boss['bossX']+14,boss['bossY']-58)
            try:
                page.wait_for_function('(v)=>RIKITU_TEST_STATE().bossHP===v',arg=hp,timeout=1400)
                break
            except Exception:
                if attempt==3:raise AssertionError(f'Boss stomp failed: {state(page)}')
    move(page,10160,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    before=state(page)
    assert before['bought']==2 and before['maxHearts']==5 and before['wallet']>=200,before
    assert page.evaluate('RIKITU_TEST_SHOP(720,0,10).unlocked')==3
    page.keyboard.press('b')
    page.wait_for_function('()=>RIKITU_TEST_STATE().bought===3',timeout=5000)
    after=state(page)
    assert after['maxHearts']==6 and after['wallet']==before['wallet']-200,after
    page.keyboard.press('b');page.wait_for_timeout(120)
    assert state(page)['bought']==3
    data=page.evaluate('(key)=>JSON.parse(localStorage.getItem(key))',KEY)
    assert data['purchased']==3 and data['levelIndex']==9 and data['mode']=='shop',data
    assert len(data.get('records',[]))>=2,data
    assert not errors,errors
    print('PASS mountain stages 9-10, airlift, boss, Moki third heart, save migration')
    browser.close()
