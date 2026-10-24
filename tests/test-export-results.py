"""CSV pacing export: no download before completion and accurate saved rows afterward."""
from pathlib import Path
import json, os, hashlib, importlib.util
from playwright.sync_api import sync_playwright

ROOT=Path(__file__).resolve().parents[1]
ANALYZER=importlib.util.spec_from_file_location(
    'rikitu_private_pacing', ROOT/'tools/analyze_private_pacing_csv.py')
pacing=importlib.util.module_from_spec(ANALYZER)
ANALYZER.loader.exec_module(pacing)
HTML=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')
KEY='rikitu-zavodnye-tropy-checkpoint-v1'
SHIM="""(initial)=>{
 const data=Object.assign({},initial);
 Object.defineProperty(window,'localStorage',{configurable:true,value:{
   getItem(k){return Object.hasOwn(data,k)?data[k]:null},
   setItem(k,v){data[k]=String(v)},removeItem(k){delete data[k]}
 }});
}"""
save={'version':1,'levelIndex':1,'mode':'shop','wallet':0,'purchased':0,
      'stageCoins':0,'fragments':[0,1,2],'coins':[],'switches':[],
      'checkpoint':105,'hearts':3,'bossDown':True,
      'records':[{'stage':1,'seconds':402.5,'hits':2,'coins':70},
                 {'stage':2,'seconds':531.5,'hits':1,'coins':80}]}

with sync_playwright() as pw:
    exe=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=exe,args=['--no-sandbox'])
    context=browser.new_context(accept_downloads=True,viewport={'width':1200,'height':760})
    page=context.new_page()
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.evaluate(SHIM,{})
    page.set_content(HTML,wait_until='load')
    assert page.locator('#export-results').is_disabled()
    page.close()

    page=context.new_page()
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    assert page.locator('#export-results').is_enabled()
    with page.expect_download(timeout=10000) as pending:
        page.locator('#export-results').click()
    download=pending.value
    assert download.suggested_filename=='RIKITU_stage_results.csv'
    raw_csv=Path(download.path()).read_bytes()
    content=raw_csv.decode('utf-8-sig')
    # Stage164: the game's *actual downloaded bytes* must pass Stage163's
    # offline parser without a separate hand-built CSV fixture.
    validated=pacing.analyse(raw_csv, 'scripted')
    assert validated['source_sha256']==hashlib.sha256(raw_csv).hexdigest()
    assert validated['source_size_bytes']==len(raw_csv)
    assert validated['counts']=={
        'stages':2,'within':2,'below':0,'above':0,
        'total_hits':3,'total_coins':150},validated['counts']
    assert [r['active_seconds'] for r in validated['stages']]==['402.5','531.5']
    assert validated['provenance_label']=='scripted'
    assert validated['provenance_verified'] is False
    # A forged comparison must be rejected even if other columns still parse.
    damaged=raw_csv.replace('В пределах цели'.encode('utf-8'),
                            'Ниже цели'.encode('utf-8'),1)
    assert damaged!=raw_csv
    try:
        pacing.analyse(damaged, 'scripted')
    except ValueError:
        pass
    else:
        raise AssertionError('mismatched game verdict accepted by private analyser')
    lines=content.splitlines()
    assert len(lines)==3,lines
    assert '"Уровень";"Название";"Тип"' in lines[0],lines[0]
    assert '"1";"Цветочная тропа";"Обычный";"402.5";"2";"70";"360";"480";"В пределах цели"' in lines[1],lines[1]
    assert '"2";"Мельница у ручья";"С мини-боссом";"531.5";"1";"80";"480";"600";"В пределах цели"' in lines[2],lines[2]
    page.keyboard.press('r')
    page.wait_for_function('()=>RIKITU_TEST_STATE().stageRecords.length===0',timeout=5000)
    assert page.locator('#export-results').is_disabled()
    assert not errors,errors
    print('PASS CSV export + Stage163 private schema readback: synthetic rows, digest, corrupted verdict rejected, reset')
    context.close()
    browser.close()
