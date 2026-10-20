"""World IV opening, seventh-heart cap, deferred Moki upgrades and old saves."""
from pathlib import Path
import os,json
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
def state(page):return page.evaluate("RIKITU_TEST_STATE()")
def move(page,x,y):page.evaluate("(p)=>RIKITU_TEST_MOVE(...p)",[x,y])
def collect(page,coords):
    for count,(x,y) in enumerate(coords,1):
        move(page,x,y)
        page.wait_for_function("(n)=>RIKITU_TEST_STATE().fragments>=n",arg=count,timeout=5000)
def new_page(browser,save):
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    return page
with sync_playwright() as p:
    exe=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=p.chromium.launch(headless=True,executable_path=exe,args=['--no-sandbox'])
    errs=[]
    save={'version':1,'levelIndex':11,'mode':'victory','wallet':300,'purchased':3,
          'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
          'switches':[],'checkpoint':8200,'hearts':6,'bossDown':True}
    page=new_page(browser,save)
    page.on('pageerror',lambda e:errs.append(str(e)))
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==16 and all(x['coins']==90 and len(x['fragments'])==3 for x in levels)
    assert len(levels[12]['switches'])==3 and len(levels[13]['vents'])==3
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===13",timeout=5000)
    assert state(page)['bought']==3 and state(page)['maxHearts']==6
    collect(page,[(1830,350),(5230,349),(8720,352)])
    move(page,9795,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===14",timeout=5000)
    collect(page,[(1950,350),(5890,351),(9550,349)])
    move(page,11034,400)
    page.wait_for_timeout(160)
    assert state(page)['mode']=='playing',state(page)
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for retry in range(4):
            boss=state(page)
            move(page,boss['bossX']+14,boss['bossY']-58)
            try:
                page.wait_for_function("(h)=>RIKITU_TEST_STATE().bossHP===h",arg=hp,timeout=1400)
                break
            except Exception:
                if retry==3: raise AssertionError(f'Boss stomp not registered: {state(page)}')
    move(page,11034,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    before=state(page)
    assert before['wallet']>=240 and before['bought']==3 and before['maxHearts']==6,before
    page.keyboard.press('b')
    page.wait_for_function("()=>RIKITU_TEST_STATE().bought===4",timeout=5000)
    after=state(page)
    assert after['wallet']==before['wallet']-240 and after['maxHearts']==7,after
    page.keyboard.press('b');page.wait_for_timeout(110)
    assert state(page)['bought']==4 and state(page)['wallet']==after['wallet']
    page.close()
    # All four missed upgrades are purchasable one by one on the last world's first shop.
    deferred={'version':1,'levelIndex':13,'mode':'shop','wallet':720,'purchased':0,
              'stageCoins':0,'fragments':[0,1,2],'coins':[],
              'switches':[],'checkpoint':8690,'hearts':3,'bossDown':True}
    page=new_page(browser,deferred)
    page.on('pageerror',lambda e:errs.append(str(e)))
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    assert page.evaluate("RIKITU_TEST_SHOP(720,0,14).unlocked")==4
    for n in (1,2,3,4):
        page.keyboard.press('b')
        page.wait_for_function("(n)=>RIKITU_TEST_STATE().bought===n",arg=n,timeout=5000)
    s=state(page)
    assert s['wallet']==0 and s['maxHearts']==7,s
    page.keyboard.press('b');page.wait_for_timeout(110)
    assert state(page)['wallet']==0 and state(page)['bought']==4
    assert not errs,errs
    print('PASS castle stages 13-14, seventh heart cap, full deferred four-heart purchase')
    browser.close()
