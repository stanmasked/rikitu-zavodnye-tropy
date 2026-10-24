(function () {
  'use strict';
  const C=window.RikituCore;
  if(!C) throw new Error('game-core.js must load before game.js');
  const canvas=document.querySelector('#game'),ctx=canvas.getContext('2d');
  const W=960,H=540,GROUND=456;
  const sfx=()=>{}; // Sound will come from original RIKITU sound-design later.
  // v0.2: four sections, three checkpoints and exactly 90 optional coins per route.
  const coinsAlong=(spans,step=145)=>spans.flatMap(([a,b])=>{
    const out=[];for(let x=Math.max(160,a+135);x<b-80;x+=step)out.push([x,393]);return out;
  });
  const platformCoins=platforms=>platforms.flatMap(([x,y,w])=>
    [0.25,0.50,0.75].map(t=>[Math.round(x+w*t),y-46]));
  const chapterInfo=[
    {name:'Цветочная тропа',world:'Солнечная долина',width:7900,
      ground:[[0,1660],[1735,3020],[3095,4310],[4385,5790],[5865,7900]],
      platforms:[[490,390,185],[1030,383,175],[1450,386,165],[2080,385,200],[2630,388,185],
        [3380,386,195],[3560,374,165],[4040,391,195],[4740,389,190],[5230,378,190],
        [5600,394,145],[6280,382,180],[6770,388,210],[7330,388,180]],
      fragments:[[1530,345],[3650,332],[6870,347]],checkpoints:[2050,4490,6150],
      enemies:[{x:820,a:745,b:1030},{x:2450,a:2310,b:2700},{x:3270,a:3150,b:3470},
        {x:4840,a:4660,b:5070},{x:5480,a:5350,b:5640},{x:7130,a:7030,b:7320}],
      sections:[[0,'Цветущие луга'],[2000,'Лесная опушка'],[4200,'Старый ручей'],[6100,'Дорога к мосту']],
      gate:7770,boss:null,startText:'Длинная Цветочная тропа: найди три фрагмента по пути!',
      endText:'Мост восстановлен! Но у мельницы клубится странный пар. Надо проверить.',decor:'meadow'},
    {name:'Мельница у ручья',world:'Солнечная долина',width:8550,
      ground:[[0,1810],[1880,3250],[3330,4690],[4770,6050],[6130,8550]],
      platforms:[[545,385,180],[1190,388,200],[1590,379,190],[2320,381,180],
        [2850,386,185],[3590,390,180],[4210,379,210],[4470,394,180],
        [5140,383,210],[5780,384,155],[6540,386,185],[6980,382,195],[7450,382,185]],
      fragments:[[1650,340],[4275,340],[7030,343]],checkpoints:[2200,4930,6980],
      enemies:[{x:840,a:735,b:1050},{x:2520,a:2440,b:2750},{x:3710,a:3550,b:3900},
        {x:5330,a:5200,b:5520},{x:6320,a:6200,b:6530},{x:7260,a:7160,b:7440}],
      sections:[[0,'Речной берег'],[2100,'Деревянные настилы'],[4300,'Водяные колёса'],[6700,'Мельничный двор']],
      gate:8370,boss:{x:7910,hp:3,name:'Заводной Жук'},startText:'Собери три фрагмента и победи мини-босса!',
      endText:'Рикиту находит пластину с клеймом мастерской. Это точно не простая авария…',decor:'mill'}
    ,{name:'Грибная роща',world:'Солнечная долина',width:8270,
      ground:[[0,1670],[1745,3180],[3260,4790],[4870,6330],[6410,8270]],
      platforms:[[520,381,200],[1080,380,180],[1490,384,185],[2200,377,210],
        [2760,387,190],[3530,379,190],[3940,379,190],[4440,388,200],
        [5170,381,175],[5740,387,220],[6660,378,180],[7040,383,200],[7570,382,205]],
      mushrooms:[[940,GROUND],[2670,GROUND],[5660,GROUND]],
      fragments:[[1530,344],[3970,344],[7040,346]],checkpoints:[2010,4330,6520],
      enemies:[{x:860,a:760,b:1030},{x:2380,a:2280,b:2520},{x:3690,a:3490,b:3880},
        {x:5280,a:5170,b:5500},{x:6110,a:5970,b:6280},{x:7490,a:7330,b:7720}],
      sections:[[0,'Лесные поляны'],[2110,'Пружинящие грибы'],[4210,'Тихая пещера'],[6380,'Тайная тропа']],
      gate:8140,boss:null,startText:'Грибная роща: мягкие грибы подбрасывают Рикиту выше!',
      endText:'Все часы показывают 07:12. Рейна предлагает исследовать старый мост.',decor:'grove'},
    {name:'Мост шестерён',world:'Солнечная долина',width:8880,
      ground:[[0,1730],[1810,3370],[3450,4960],[5040,6610],[6690,8880]],
      platforms:[[610,380,190],[1280,378,200],[1630,383,175],[2380,378,200],
        [2910,383,185],[3680,376,195],[4220,382,210],[4710,379,180],
        [5460,382,195],[6040,379,215],[7070,382,175],[7420,376,205],[7930,381,190]],
      switches:[[2260,GROUND],[4930,GROUND],[6990,GROUND]],
      fragments:[[1580,343],[4590,339],[7420,343]],checkpoints:[2100,4570,6900],
      enemies:[{x:870,a:760,b:1020},{x:2570,a:2460,b:2850},{x:3860,a:3690,b:4060},
        {x:5510,a:5400,b:5690},{x:6360,a:6190,b:6550},{x:7620,a:7510,b:7840}],
      sections:[[0,'Первый пролёт'],[2190,'Механические опоры'],[4440,'Три рычага'],[6660,'Ворота моста']],
      gate:8740,boss:{x:8290,hp:3,name:'Страж моста'},
      startText:'Мост шестерён: включи рычаги, найди три фрагмента и одолей Стража!',
      endText:'Приказ закрыть мост отдали заранее. Рикиту с Рейной идут в Заводной город.',decor:'bridge'}
    ,{name:'Улица мастерских',world:'Заводной город',width:9140,
      ground:[[0,1790],[1865,3450],[3530,5020],[5095,6650],[6730,9140]],
      platforms:[[510,385,190],[1080,381,190],[1550,383,205],[2250,379,195],[2840,384,210],
        [3710,386,180],[4170,380,210],[4660,385,180],[5480,383,210],
        [5960,379,205],[7120,380,185],[7580,385,190],[8160,383,220]],
      conveyors:[[1070,430,300,48],[3890,430,300,-42],[6960,430,300,52]],
      fragments:[[1700,345],[4590,346],[7780,345]],checkpoints:[2200,4760,6920],
      enemies:[{x:780,a:670,b:1050},{x:2490,a:2370,b:2710},{x:3920,a:3760,b:4080},
        {x:5680,a:5540,b:5830},{x:6500,a:6360,b:6620},{x:8480,a:8330,b:8630}],
      sections:[[0,'Часовая улица'],[2280,'Фабричные дворы'],[4580,'Улица посылок'],[6890,'Площадь мастерских']],
      gate:8950,boss:null,startText:'Новый мир: Заводной город! Найди три фрагмента и накладную Кэша.',
      endText:'Кэш показывает накладную: все грузы везли через Подземную доставку.',decor:'city'},
    {name:'Подземная доставка',world:'Заводной город',width:9910,
      ground:[[0,1870],[1940,3550],[3630,5280],[5360,7020],[7100,9910]],
      platforms:[[520,382,185],[1230,378,210],[1720,380,205],[2440,383,200],[3080,379,190],
        [3910,382,205],[4680,378,195],[5160,386,175],[5910,383,190],
        [6520,379,210],[7550,385,190],[8170,381,195],[8780,383,205]],
      conveyors:[[590,431,310,42],[3980,430,310,-48],[7480,430,330,50]],
      vents:[[1350,GROUND],[4740,GROUND],[7910,GROUND]],
      fragments:[[1810,342],[5220,340],[8120,345]],checkpoints:[2300,5060,7360],
      enemies:[{x:780,a:680,b:1030},{x:2680,a:2550,b:2930},{x:3820,a:3700,b:4000},
        {x:5550,a:5430,b:5690},{x:6860,a:6770,b:6990},{x:8420,a:8260,b:8610}],
      sections:[[0,'Транспортный тоннель'],[2390,'Пневматические шахты'],[4870,'Зона сортировки'],[7350,'Главный грузовой склад']],
      gate:9730,boss:{x:9210,hp:3,name:'Пыльный бурильщик'},
      startText:'Подземная доставка: паровые подъёмники и конвейеры помогут найти детали.',
      endText:'Магистр Пыль подтверждает: тоннели закрыли по странному приказу.',decor:'tunnel'}
    ,{name:'Часовая площадь',world:'Заводной город',width:9470,
      ground:[[0,1820],[1895,3520],[3600,5220],[5300,6840],[6920,9470]],
      platforms:[[510,383,200],[1180,379,190],[1590,383,205],[2440,382,180],[3130,378,200],
        [3900,385,205],[4560,379,190],[5030,383,180],[5650,381,220],
        [6370,385,190],[7360,382,205],[8050,379,190],[8670,381,210]],
      clockPads:[[1350,GROUND],[4140,GROUND],[7770,GROUND]],
      conveyors:[[2600,430,310,40],[6030,430,300,-42]],
      fragments:[[1740,345],[4760,345],[8110,344]],checkpoints:[2240,4930,7240],
      enemies:[{x:850,a:720,b:1070},{x:2700,a:2590,b:2910},{x:4000,a:3830,b:4190},
        {x:5560,a:5450,b:5750},{x:6590,a:6470,b:6800},{x:8410,a:8280,b:8560}],
      sections:[[0,'Колокольная улица'],[2360,'Площадь механиков'],[4720,'Архивный проход'],[7040,'Старый регистратор']],
      gate:9290,boss:null,startText:'Часовая площадь: циферблаты поднимают Рикиту без ограничения времени.',
      endText:'Регистратор сохранил время 07:12. Кто-то позднее исправил записи!',decor:'clock'},
    {name:'Склад приказов',world:'Заводной город',width:10180,
      ground:[[0,1900],[1980,3700],[3780,5520],[5600,7450],[7530,10180]],
      platforms:[[480,381,195],[1250,384,205],[1690,380,190],[2390,379,205],
        [3120,385,195],[4100,381,200],[4770,383,205],[5390,380,185],
        [6160,382,195],[6840,379,210],[7660,384,200],[8310,381,205],[8970,379,190]],
      conveyors:[[890,430,315,45],[4320,430,300,-48],[7730,430,330,47]],
      switches:[[2120,GROUND],[5390,GROUND],[7560,GROUND]],
      fragments:[[1820,344],[5080,343],[8450,343]],checkpoints:[2380,5230,7710],
      enemies:[{x:850,a:740,b:1090},{x:2720,a:2590,b:2870},{x:4250,a:4100,b:4480},
        {x:5840,a:5720,b:6080},{x:7230,a:7050,b:7410},{x:8560,a:8460,b:8820}],
      sections:[[0,'Приёмная склада'],[2460,'Линия сортировки'],[4990,'Комната печатей'],[7650,'Хранилище распоряжений']],
      gate:9990,boss:{x:9470,hp:3,name:'Патрульный автомат'},
      startText:'Склад приказов: активируй печати и найди три детали до выхода.',
      endText:'Рэтчет предъявил приказ центральной службы. Нить расследования ведёт в Забытые горы.',decor:'depot'}
    ,{name:'Туманный перевал',world:'Забытые горы',width:9520,
      ground:[[0,1830],[1910,3510],[3590,5260],[5340,7030],[7110,9520]],
      platforms:[[560,384,190],[1240,381,210],[1680,385,180],[2410,379,195],
        [3020,382,200],[3850,383,210],[4480,379,200],[5030,382,185],
        [5720,380,205],[6460,384,210],[7430,381,185],[8060,379,195],[8650,384,200]],
      gusts:[[1240,GROUND],[3880,GROUND],[7520,GROUND]],
      fragments:[[1740,344],[4730,344],[8170,345]],checkpoints:[2280,4880,7290],
      enemies:[{x:840,a:730,b:1040},{x:2610,a:2510,b:2880},{x:4170,a:4060,b:4370},
        {x:5660,a:5510,b:5820},{x:6870,a:6710,b:6980},{x:8460,a:8330,b:8680}],
      sections:[[0,'Туманная дорога'],[2310,'Скальный уступ'],[4810,'Старая метеостанция'],[7210,'Перевалочный пункт']],
      gate:9330,boss:null,startText:'Туманный перевал: восходящие потоки помогают перепрыгивать скалы.',
      endText:'В горной накладной указан необычный груз, направленный на Облачную пристань.',decor:'mountain'},
    {name:'Облачная пристань',world:'Забытые горы',width:10320,
      ground:[[0,1940],[2010,3820],[3900,5660],[5740,7670],[7750,10320]],
      platforms:[[580,383,205],[1180,379,190],[1710,382,210],[2480,382,205],
        [3180,378,195],[4260,382,195],[4850,381,205],[5470,384,185],
        [6350,380,210],[7050,383,195],[8040,381,190],[8680,379,210],[9260,384,200]],
      gusts:[[980,GROUND],[4300,GROUND],[7120,GROUND],[8620,GROUND]],
      conveyors:[[2860,430,300,38],[6520,430,300,-38]],
      fragments:[[1900,345],[5210,342],[8740,343]],checkpoints:[2470,5420,8020],
      enemies:[{x:870,a:760,b:1010},{x:2700,a:2550,b:2850},{x:4420,a:4300,b:4650},
        {x:6130,a:6000,b:6340},{x:7380,a:7210,b:7550},{x:9030,a:8900,b:9240}],
      sections:[[0,'Подъём к причалу'],[2560,'Облачные платформы'],[5120,'Зал погрузки'],[7920,'Лётная площадка']],
      gate:10140,boss:{x:9650,hp:3,name:'Ветряной автомат'},
      startText:'Облачная пристань: используй безопасные восходящие потоки, чтобы найти детали.',
      endText:'Капитан Буря сообщил: часовое оборудование везли по закрытому маршруту.',decor:'skyport'}
    ,{name:'Кристальные пещеры',world:'Забытые горы',width:9820,
      ground:[[0,1850],[1930,3630],[3710,5470],[5550,7250],[7330,9820]],
      platforms:[[540,381,185],[1100,378,195],[1560,383,205],[2350,381,190],
        [3060,385,195],[3890,379,210],[4560,383,190],[5090,380,190],
        [5900,380,215],[6620,383,205],[7500,380,190],[8190,383,210],[8930,380,190]],
      crystals:[[1120,GROUND],[3980,GROUND],[7650,GROUND]],
      fragments:[[1770,343],[4930,341],[8440,343]],checkpoints:[2370,5050,7470],
      enemies:[{x:850,a:710,b:1020},{x:2690,a:2510,b:2870},{x:4060,a:3920,b:4250},
        {x:5800,a:5650,b:5950},{x:6990,a:6810,b:7190},{x:8610,a:8470,b:8800}],
      sections:[[0,'Сверкающий вход'],[2430,'Зал кристаллов'],[4820,'Архив древних мастеров'],[7340,'Подземный ручей']],
      gate:9620,boss:null,startText:'Кристальные пещеры: светящиеся кристаллы мягко подбрасывают вверх.',
      endText:'Леди Морин показала старинный чертёж: Сердце дорог специально разъединяет шестерни при аварии.',decor:'crystal'},
    {name:'Шахта молчания',world:'Забытые горы',width:10700,
      ground:[[0,1910],[1990,3860],[3940,5900],[5980,7970],[8050,10700]],
      platforms:[[540,380,195],[1260,383,190],[1810,378,200],[2510,380,200],
        [3260,385,195],[4150,380,205],[4800,383,210],[5450,381,190],
        [6300,381,210],[7100,380,200],[8200,380,190],[8870,383,195],[9470,381,200]],
      conveyors:[[730,430,290,43],[4610,430,300,-44],[8290,430,330,46]],
      vents:[[1480,GROUND],[5140,GROUND],[7580,GROUND]],
      switches:[[2590,GROUND],[6030,GROUND],[8260,GROUND]],
      fragments:[[1810,343],[5600,342],[9130,344]],checkpoints:[2520,5490,8200],
      enemies:[{x:840,a:730,b:1040},{x:2800,a:2680,b:3020},{x:4300,a:4150,b:4500},
        {x:6660,a:6530,b:6850},{x:7850,a:7740,b:7970},{x:9110,a:8970,b:9270}],
      sections:[[0,'Шахтные ворота'],[2740,'Сортировка руды'],[5440,'Термальный архив'],[8180,'Площадка буровых машин']],
      gate:10530,boss:{x:9930,hp:3,name:'Буровой караул'},
      startText:'Шахта молчания: используй пар и конвейеры, затем останови Буровой караул.',
      endText:'Шахтный регистратор подтвердил опасное ускорение Сердца дорог. Нужно найти способ безопасного ремонта.',decor:'mine'}
    ,{name:'Замковые сады',world:'Замок механизмов',width:9950,
      ground:[[0,1950],[2030,3890],[3970,5860],[5940,7750],[7830,9950]],
      platforms:[[570,381,205],[1240,383,195],[1770,380,190],[2520,381,210],
        [3240,379,190],[4230,382,200],[4880,379,210],[5570,381,185],
        [6410,379,205],[7150,383,190],[8060,381,195],[8620,380,205],[9150,383,210]],
      mushrooms:[[1100,GROUND],[4780,GROUND],[8190,GROUND]],
      switches:[[2330,GROUND],[6110,GROUND],[8080,GROUND]],
      fragments:[[1840,344],[5240,342],[8730,346]],checkpoints:[2520,5280,7950],
      enemies:[{x:890,a:750,b:1070},{x:2790,a:2660,b:2960},{x:4380,a:4230,b:4630},
        {x:6580,a:6440,b:6760},{x:7640,a:7500,b:7740},{x:9060,a:8920,b:9250}],
      sections:[[0,'Розарий башен'],[2510,'Аллея статуй'],[5060,'Сад заводных ворот'],[7640,'Путь в литейную']],
      gate:9780,boss:null,startText:'Замковые сады: ищи три фрагмента, мягкие грибы и безопасные механические рычаги.',
      endText:'Барон Грейсток раскрыл свои сделки и указал дорогу к мастерской Моториуса.',decor:'gardens'},
    {name:'Литейная Сердца',world:'Замок механизмов',width:11200,
      ground:[[0,2040],[2120,4220],[4300,6420],[6500,8660],[8740,11200]],
      platforms:[[530,382,200],[1230,379,205],[1890,383,190],[2690,380,205],
        [3520,381,195],[4700,382,210],[5310,379,205],[6000,380,195],
        [6950,381,205],[7680,383,195],[8800,379,190],[9450,383,195],[10140,380,205]],
      conveyors:[[800,430,310,45],[4670,430,300,-42],[8940,430,320,47]],
      vents:[[1570,GROUND],[5420,GROUND],[8380,GROUND]],
      switches:[[2380,GROUND],[6500,GROUND],[8770,GROUND]],
      fragments:[[1960,343],[5900,344],[9560,342]],checkpoints:[2600,5800,8690],
      enemies:[{x:870,a:740,b:1040},{x:2990,a:2790,b:3160},{x:4740,a:4540,b:4920},
        {x:6930,a:6710,b:7130},{x:8350,a:8200,b:8590},{x:9590,a:9460,b:9800}],
      sections:[[0,'Зал руды'],[2740,'Кузнечные механизмы'],[5470,'Литейные конвейеры'],[8760,'Главная сборочная']],
      gate:11020,boss:{x:10470,hp:3,name:'Сборочный исполин'},
      startText:'Литейная Сердца: поднимайся на пару, обходи машины и останови Сборочного исполина.',
      endText:'Моториус показал ограничитель скорости. Теперь Сердце дорог можно восстановить безопасно.',decor:'foundry'}
    ,{name:'Галерея часов',world:'Замок механизмов',width:10300,
      ground:[[0,1970],[2050,4070],[4150,6120],[6200,8160],[8240,10300]],
      platforms:[[520,381,205],[1160,380,205],[1750,382,195],[2590,380,210],
        [3350,383,205],[4480,380,200],[5090,382,205],[5680,380,195],
        [6620,381,210],[7320,384,190],[8490,380,205],[9090,382,195],[9640,381,205]],
      clockPads:[[1150,GROUND],[4500,GROUND],[8470,GROUND]],
      switches:[[2320,GROUND],[6360,GROUND],[8550,GROUND]],
      fragments:[[1800,345],[5490,343],[9290,343]],checkpoints:[2500,5430,8210],
      enemies:[{x:870,a:730,b:1050},{x:2810,a:2690,b:3000},{x:4550,a:4420,b:4770},
        {x:6660,a:6540,b:6900},{x:7960,a:7810,b:8130},{x:9390,a:9260,b:9550}],
      sections:[[0,'Коридор маятников'],[2580,'Башня циферблатов'],[5250,'Архив времени'],[8220,'Последняя запись']],
      gate:10120,boss:null,startText:'Галерея часов: циферблаты помогают добраться до записей без таймера.',
      endText:'Запись 07:12 доказывает: Хроном сам запустил аварийную остановку Сердца дорог.',decor:'gallery'},
    {name:'Зал Сердца',world:'Замок механизмов',width:11750,
      ground:[[0,2160],[2240,4490],[4570,6830],[6910,9110],[9190,11750]],
      platforms:[[520,380,195],[1230,381,200],[1820,383,195],[2860,380,210],
        [3590,381,205],[4760,382,210],[5550,380,190],[6150,382,195],
        [7390,381,210],[8120,380,200],[9160,382,195],[9930,380,195],[10740,380,210]],
      conveyors:[[880,430,330,44],[5070,430,300,-44],[9200,430,310,45]],
      vents:[[1700,GROUND],[5640,GROUND],[8580,GROUND]],
      clockPads:[[3310,GROUND],[7410,GROUND]],
      fragments:[[2020,342],[6200,342],[9950,342]],checkpoints:[2930,6260,9260],
      enemies:[{x:920,a:780,b:1130},{x:3090,a:2920,b:3310},{x:4850,a:4680,b:5000},
        {x:6990,a:6870,b:7200},{x:9030,a:8890,b:9120},{x:10120,a:9990,b:10370}],
      sections:[[0,'Подход к Сердцу'],[3030,'Каналы энергии'],[6000,'Защитный контур'],[9160,'Центральный механизм']],
      gate:11560,boss:{x:10950,hp:3,name:'Хранитель минут'},
      startText:'Зал Сердца: собери последнюю шестерёнку и отключи Хранителя минут.',
      endText:'Рикиту и Хроном запускают безопасное Сердце дорог. Часы идут: 07:13.',decor:'heart'}
  ].map(d=>{
    const coins=[...coinsAlong(d.ground),...platformCoins(d.platforms)];
    let i=0;
    while(coins.length<C.SETTINGS.coinsPerStage){
      const [a,b]=d.ground[i%d.ground.length],x=a+65+Math.floor(i/d.ground.length)*47;
      if(x<b-30&&!coins.some(c=>Math.abs(c[0]-x)<20&&Math.abs(c[1]-425)<20))coins.push([x,425]);
      i++;if(i>5000)throw new Error('Could not place target coins');
    }
    return {...d,coins:coins.slice(0,C.SETTINGS.coinsPerStage)};
  });
  const keys=new Set(); const touch={left:false,right:false,jump:false};
  let mode='title',levelIndex=0,level=null,p=null,cam=0,frame=0,lastTime=0,acc=0,toast='',toastTime=0,storyText='',bossHint=false;
  const fixed=1/60;
  let stageCoins=0,allCoins=0,purchasedHearts=0,shopStage=0;
  let stageSeconds=0,stageHits=0,stageRecords=[];
  const lifeCapacity=()=>C.SETTINGS.startingHearts+purchasedHearts;
  const clamp=(v,a,b)=>Math.min(b,Math.max(a,v));
  const coinBox=(x,y)=>({x:x-12,y:y-12,w:24,h:24});
  const formatClock=seconds=>`${String(Math.floor(seconds/60)).padStart(2,'0')}:${String(Math.floor(seconds%60)).padStart(2,'0')}`;

  const SAVE_KEY='rikitu-zavodnye-tropy-checkpoint-v1';
  const allowedSaveModes=new Set(['playing','between','shop','victory']);
  function readSave(){
    try{
      const raw=localStorage.getItem(SAVE_KEY);
      if(!raw)return null;
      const s=JSON.parse(raw);
      const int=x=>Number.isInteger(x);
      if(!s||s.version!==1||!allowedSaveModes.has(s.mode))return null;
      if(!int(s.levelIndex)||s.levelIndex<0||s.levelIndex>=chapterInfo.length)return null;
      // v0.5 ended after level 4; migrate its victory screen into the level-5 transition.
      if(s.mode==='victory'&&s.levelIndex===3&&chapterInfo.length>4)s.mode='between';
      if(s.mode==='victory'&&s.levelIndex===5&&chapterInfo.length>6)s.mode='between';
      if(s.mode==='victory'&&s.levelIndex===7&&chapterInfo.length>8)s.mode='between';
      if(s.mode==='victory'&&s.levelIndex===9&&chapterInfo.length>10)s.mode='between';
      if(s.mode==='victory'&&s.levelIndex===11&&chapterInfo.length>12)s.mode='between';
      if(s.mode==='victory'&&s.levelIndex===13&&chapterInfo.length>14)s.mode='between';
      if(!int(s.wallet)||s.wallet<0||s.wallet>chapterInfo.length*C.SETTINGS.coinsPerStage)return null;
      if(!int(s.purchased)||s.purchased<0||s.purchased>4||s.purchased>C.unlockedUpgrades(s.levelIndex+1))return null;
      if(!Array.isArray(s.fragments)||!Array.isArray(s.coins)||!Array.isArray(s.switches))return null;
      const d=chapterInfo[s.levelIndex];
      const validIds=(ids,max)=>ids.length<=max&&new Set(ids).size===ids.length&&ids.every(v=>int(v)&&v>=0&&v<max);
      if(!validIds(s.fragments,3)||!validIds(s.coins,d.coins.length)||!validIds(s.switches,(d.switches||[]).length))return null;
      if(!int(s.stageCoins)||s.stageCoins!==s.coins.length)return null; // Wallet may be lower after Moki purchases.
      if(!int(s.checkpoint)||![105,...d.checkpoints].includes(s.checkpoint))return null;
      if(!int(s.hearts)||s.hearts<1||s.hearts>3+s.purchased)return null;
      // Optional fields: existing checkpoint saves created before the pacing update remain valid.
      if(s.seconds!==undefined&&(!Number.isFinite(s.seconds)||s.seconds<0||s.seconds>36000))return null;
      if(s.hits!==undefined&&(!int(s.hits)||s.hits<0||s.hits>9999))return null;
      // Completion history was introduced after the original checkpoint format.
      if(s.records!==undefined){
        if(!Array.isArray(s.records)||s.records.length>chapterInfo.length)return null;
        const used=new Set();
        for(const r of s.records){
          if(!r||!int(r.stage)||r.stage<1||r.stage>s.levelIndex+1||used.has(r.stage))return null;
          if(!Number.isFinite(r.seconds)||r.seconds<0||r.seconds>36000)return null;
          if(!int(r.hits)||r.hits<0||r.hits>9999)return null;
          if(!int(r.coins)||r.coins<0||r.coins>C.SETTINGS.coinsPerStage)return null;
          used.add(r.stage);
        }
      }
      if(typeof s.bossDown!=='boolean')return null;
      if(s.mode==='shop'&&(!C.isShopStage(s.levelIndex+1)||!s.bossDown))return null;
      if(s.mode==='between'&&s.levelIndex>=chapterInfo.length-1)return null;
      if(s.mode==='victory'&&s.levelIndex!==chapterInfo.length-1)return null;
      return s;
    }catch(e){return null;}
  }
  let startupSave=readSave();
  function persist(){
    if(!level||!p||!allowedSaveModes.has(mode))return;
    const data={version:1,levelIndex,mode,wallet:allCoins,purchased:purchasedHearts,
      stageCoins,fragments:level.fragments.filter(x=>x.got).map(x=>x.id),
      coins:level.coins.filter(x=>x.got).map(x=>x.id),
      switches:level.switches.map((x,i)=>x.on?i:-1).filter(x=>x>=0),
      checkpoint:level.activeCheckpoint,hearts:p.hearts,
      seconds:Math.round(stageSeconds*10)/10,hits:stageHits,records:stageRecords.map(x=>({...x})),
      bossDown:!!level.boss&&!level.boss.alive};
    try{localStorage.setItem(SAVE_KEY,JSON.stringify(data));startupSave=data;}catch(e){}
  }
  function resume(){
    const s=startupSave;
    if(!s)return false;
    newLevel(s.levelIndex,false);
    allCoins=s.wallet;purchasedHearts=s.purchased;stageCoins=s.stageCoins;
    stageSeconds=s.seconds??0;stageHits=s.hits??0;stageRecords=(s.records||[]).map(x=>({...x}));
    for(const item of level.fragments)item.got=s.fragments.includes(item.id);
    for(const item of level.coins)item.got=s.coins.includes(item.id);
    level.switches.forEach((sw,i)=>{sw.on=s.switches.includes(i)});
    level.activeCheckpoint=s.checkpoint;
    level.checkpointCount=level.checkpoints.filter(cp=>cp<=s.checkpoint).length;
    p.spawnX=s.checkpoint;p.x=p.spawnX;p.y=GROUND-p.h;p.hearts=s.hearts;p.inv=1;
    if(s.bossDown&&level.boss){level.boss.alive=false;level.boss.hp=0;}
    mode=s.mode;shopStage=mode==='shop'?levelIndex+1:0;
    announce('Игра восстановлена с контрольной точки',2.5);
    updateHud();persist();return true;
  }

  function newLevel(index,save=true) {
    levelIndex=index;
    const d=chapterInfo[index];
    level={...d,
      platforms:d.platforms.map(a=>({x:a[0],y:a[1],w:a[2],h:16})),
      ground:d.ground.map(a=>({x:a[0],y:GROUND,w:a[1]-a[0],h:H-GROUND+180})),
      fragments:d.fragments.map((a,i)=>({id:i,x:a[0],y:a[1],w:25,h:25,got:false})),
      coins:d.coins.map((a,i)=>({id:i,x:a[0],y:a[1],got:false})),
      mushrooms:(d.mushrooms||[]).map(([x,y])=>({x,y})),
      conveyors:(d.conveyors||[]).map(([x,y,w,speed])=>({x,y,w,speed})),
      vents:(d.vents||[]).map(([x,y])=>({x,y})),
      clockPads:(d.clockPads||[]).map(([x,y])=>({x,y})),
      gusts:(d.gusts||[]).map(([x,y])=>({x,y})),
      crystals:(d.crystals||[]).map(([x,y])=>({x,y})),
      switches:(d.switches||[]).map(([x,y])=>({x,y,on:false})),
      enemies:d.enemies.map((e,i)=>({id:i,x:e.x,y:GROUND-28,w:33,h:28,a:e.a,b:e.b,dir:i%2?1:-1,dead:false})),
      boss:d.boss?{x:d.boss.x,y:GROUND-57,w:72,h:57,center:d.boss.x,hp:d.boss.hp,name:d.boss.name,hit:0,dir:1,alive:true}:null,
      activeCheckpoint:105, checkpointCount:0,
    };
    stageCoins=0;stageSeconds=0;stageHits=0;previousJump=false;
    p={x:105,y:GROUND-C.SETTINGS.playerHeight,w:C.SETTINGS.playerWidth,h:C.SETTINGS.playerHeight,
      vx:0,vy:0,grounded:false,coyote:0,jumpBuffer:0,inv:0,hearts:lifeCapacity(),
      spawnX:105, facing:1};
    cam=0;bossHint=false;toast=d.startText;toastTime=4.8;
    mode='playing';
    updateHud();if(save)persist();
  }
  function updateHud(){
    const fragmentCount=level?.fragments.filter(x=>x.got).length||0;
    const h=document.querySelector('#hud');
    h.innerHTML=`<span>♥ ${p?p.hearts:3}/${lifeCapacity()}</span><span>🟡 ${allCoins} <small>кошелёк</small></span><span>◆ ${fragmentCount}/3 <small>фрагмента</small></span><span>⏱ ${formatClock(stageSeconds)} <small>потерь здоровья: ${stageHits}</small></span><span class="level-mark">${level ? `${levelIndex+1}/${chapterInfo.length} — ${level.name} · ${Math.min(100,Math.round((p?.x||0)/level.width*100))}%` : 'RIKITU'}</span>`;
    document.querySelector('#export-results').disabled=stageRecords.length===0;
  }
  function announce(t,time=2.5){toast=t;toastTime=time;}
  function damage(){
    if(p.inv>0 || mode!=='playing')return;
    p.hearts=C.heartAfterDamage(p.hearts);p.inv=C.SETTINGS.invincibility;
    stageHits++;
    announce(p.hearts?'Осторожнее!':'Не беда! Вернёмся к контрольной точке.',1.6);
    if(p.hearts===0){p.hearts=lifeCapacity();respawn();}
    updateHud();persist();
  }
  function respawn(){
    p.x=p.spawnX;p.y=GROUND-p.h;p.vx=0;p.vy=0;p.inv=1.4;p.grounded=false;
    // Collected parts and coins persist, to keep the game accessible.
  }
  function jumpPressed(){return keys.has('Space')||keys.has('ArrowUp')||keys.has('KeyW')||touch.jump||gamepadJump;}
  let gamepadJump=false,gamepadX=0,gamepadRun=false,gamepadPause=false,previousJump=false;
  function readGamepad(){
    let pads=[];try{pads=navigator.getGamepads?navigator.getGamepads():[];}catch(e){}
    const gp=Array.from(pads).find(g=>g&&g.connected);
    gamepadX=gp?gp.axes[0]||0:0;
    // Standard Gamepad mapping: D-pad left/right override a neutral or analog stick axis.
    const dpadLeft=!!(gp&&gp.buttons[14]?.pressed);
    const dpadRight=!!(gp&&gp.buttons[15]?.pressed);
    if(dpadLeft!==dpadRight)gamepadX=dpadLeft?-1:1;
    gamepadJump=!!(gp&&(gp.buttons[0]?.pressed||gp.buttons[1]?.pressed));
    gamepadRun=!!(gp&&gp.buttons[5]?.pressed); // RB/R1, held for faster movement.
    const pause=!!(gp&&gp.buttons[9]?.pressed);
    if(pause&&!gamepadPause)togglePause(); gamepadPause=pause;
  }
  function update(dt){
    frame++;readGamepad();
    if(mode!=='playing')return;
    stageSeconds+=dt;
    const d=C.SETTINGS;
    if(toastTime>0)toastTime-=dt;
    p.inv=Math.max(0,p.inv-dt);
    const left=keys.has('ArrowLeft')||keys.has('KeyA')||touch.left||gamepadX<-.30;
    const right=keys.has('ArrowRight')||keys.has('KeyD')||touch.right||gamepadX>.30;
    p.vx=((right?1:0)-(left?1:0))*((keys.has('ShiftLeft')||gamepadRun)?d.runSpeed:d.walkSpeed);
    if(p.vx)p.facing=Math.sign(p.vx);
    const jumpNow=jumpPressed();
    p.jumpBuffer=(jumpNow&&!previousJump)?d.jumpBuffer:Math.max(0,p.jumpBuffer-dt);
    previousJump=jumpNow;
    p.coyote=p.grounded?d.coyoteTime:Math.max(0,p.coyote-dt);
    if(p.jumpBuffer>0&&C.playerCanJump(p)) {
      p.vy=-d.jumpSpeed;p.grounded=false;p.coyote=0;p.jumpBuffer=0;sfx();
    }
    const oldBottom=p.y+p.h;
    p.x=clamp(p.x+p.vx*dt,0,level.width-p.w);
    p.vy=Math.min(1050,p.vy+d.gravity*dt);
    p.y+=p.vy*dt;p.grounded=false;
    const solids=[...level.ground,...level.platforms];
    for(const b of solids){
      if(p.x+p.w <= b.x+3||p.x>=b.x+b.w-3)continue;
      if(p.vy>=0 && oldBottom<=b.y+10 && p.y+p.h>=b.y && p.y<=b.y){
        p.y=b.y-p.h;p.vy=0;p.grounded=true;
      }
    }
    // Gentle moving floor belts and steam updrafts: no new action buttons.
    if(p.grounded){
      for(const belt of level.conveyors){
        if(p.x+p.w>belt.x&&p.x<belt.x+belt.w){
          p.x=clamp(p.x+belt.speed*dt,0,level.width-p.w);break;
        }
      }
      for(const vent of level.vents){
        if(p.x+p.w>vent.x-18&&p.x<vent.x+52){
          p.vy=-705;p.grounded=false;announce('Паровой подъёмник!',1.2);break;
        }
      }
    }
    if(p.grounded){
      for(const dial of level.clockPads){
        if(p.x+p.w>dial.x-16&&p.x<dial.x+54){
          p.vy=-675;p.grounded=false;
          announce('Часовая пружина! Никакого таймера.',1.4);break;
        }
      }
    }
    // Gust vents are forgiving lifts: never force horizontal drift towards a pit.
    if(p.grounded){
      for(const gust of level.gusts){
        if(p.x+p.w>gust.x-24&&p.x<gust.x+66){
          p.vy=-690;p.grounded=false;announce('Тёплый восходящий поток!',1.3);break;
        }
      }
    }
    if(p.grounded){
      for(const crystal of level.crystals){
        if(p.x+p.w>crystal.x-20&&p.x<crystal.x+60){
          p.vy=-690;p.grounded=false;announce('Кристаллическая пружина!',1.2);break;
        }
      }
    }
    // Spring mushrooms are forgiving, optional vertical shortcuts in the Grove.
    if(p.grounded){
      for(const m of level.mushrooms){
        if(p.x+p.w>m.x-20&&p.x<m.x+64&&Math.abs(p.y+p.h-m.y)<7){
          p.vy=-730;p.grounded=false;announce('Пружинящий гриб!',1.0);break;
        }
      }
    }
    // One-touch bridge levers animate the route; only fragments gate the exit.
    for(const sw of level.switches){
      if(!sw.on&&p.x+p.w>sw.x&&p.x<sw.x+34&&Math.abs(p.y+p.h-sw.y)<75){
        sw.on=true;announce('Рычаг включён! Мост оживает.',1.7);
      }
    }
    if(p.y>H+150){damage();respawn();}
    let checkpointChanged=false;
    for(const cp of level.checkpoints){
      if(p.x>=cp && level.activeCheckpoint<cp){
        level.activeCheckpoint=cp;level.checkpointCount++;p.spawnX=cp;checkpointChanged=true;
        announce(`✓ Контрольная точка ${level.checkpointCount}/3. Находки сохраняются!`,2.9);
      }
    }
    let hudChanged=false;
    for(const part of level.fragments){
      if(part.got)continue;
      if(C.boxHit(p,{x:part.x-20,y:part.y-30,w:65,h:72})){
        part.got=true;hudChanged=true;
        const count=level.fragments.filter(q=>q.got).length;
        announce(count===3?'◆ Деталь собрана! Путь к выходу открыт.':`◆ Найден фрагмент ${count} из 3!`,3.0);
      }
    }
    for(const coin of level.coins){
      if(coin.got)continue;
      if(C.boxHit(p,coinBox(coin.x,coin.y))){coin.got=true;stageCoins++;allCoins++;hudChanged=true;}
    }
    for(const e of level.enemies){
      if(e.dead)continue;
      e.x+=e.dir*43*dt;
      if(e.x<e.a||e.x>e.b){e.x=clamp(e.x,e.a,e.b);e.dir*=-1;}
      if(C.boxHit(p,e)){
        if(C.stompFromAbove(oldBottom,p.y+p.h,e.y,p.vy>=0)){
          e.dead=true;p.vy=-360;announce('Отличный прыжок!',1.1);
        }else damage();
      }
    }
    if(level.boss&&level.boss.alive){
      let boss=level.boss;
      boss.hit=Math.max(0,boss.hit-dt);
      boss.x+=boss.dir*48*dt;
      if(boss.x<boss.center-80||boss.x>boss.center+80)boss.dir*=-1;
      if(p.x>boss.center-380&&!bossHint){bossHint=true;announce(`Мини-босс: ${boss.name}. Прыгни сверху трижды!`,4.5);}
      if(C.boxHit(p,boss)){
        if(boss.hit<=0&&C.stompFromAbove(oldBottom,p.y+p.h,boss.y,p.vy>=0)){
          boss.hp--;boss.hit=0.85;p.vy=-490;p.y=boss.y-p.h-1;
          announce(boss.hp?`${boss.name}: осталось ${boss.hp} попаданий!`:`${boss.name} остановлен! Иди к выходу.`,2.3);
          if(boss.hp===0){boss.alive=false;persist();}
        }else if(boss.hit<=0) damage();
      }
    }
    if(hudChanged)updateHud();
    if(hudChanged||checkpointChanged)persist();
    if(p.x+p.w >level.gate-30){
      if(C.canExit(level.fragments.filter(x=>x.got).length,!!level.boss,!level.boss||!level.boss.alive)){
        if(!stageRecords.some(r=>r.stage===levelIndex+1))
          stageRecords.push({stage:levelIndex+1,seconds:Math.round(stageSeconds*10)/10,hits:stageHits,coins:stageCoins});
        storyText=level.endText; if(C.isShopStage(levelIndex+1)){mode='shop';shopStage=levelIndex+1;} else mode=levelIndex===chapterInfo.length-1?'victory':'between';
        updateHud();persist();
      } else if(toastTime<=0){
        const missing=level.fragments.filter(x=>!x.got);
        if(missing.length){
          const first=missing[0];
          const section=level.sections.reduce((found,s)=>s[0]<=first.x?s[1]:found,level.sections[0][1]);
          announce(`Фрагмент остался позади — участок «${section}». Вернись!`,3.3);
        }else announce('Сначала победи мини-босса!',2.6);
      }
    }
    const target=clamp(p.x-W*.37,0,Math.max(0,level.width-W));
    if(frame%15===0)updateHud();
    cam+=(target-cam)*clamp(8*dt,0,1);
  }
  function rounded(x,y,w,h,r,color){ctx.fillStyle=color;ctx.beginPath();ctx.roundRect(x,y,w,h,r);ctx.fill();}
  function text(t,x,y,size=22,color='#fff',align='left',weight='700'){
    ctx.textAlign=align;ctx.font=`${weight} ${size}px system-ui, Arial, sans-serif`;ctx.fillStyle=color;ctx.fillText(t,x,y);
  }
  function clouds(offset){
    ctx.save();ctx.translate(-offset*.15,0);
    for(let i=0;i<18;i++){
      let x=40+i*235,y=75+(i*31)%95;
      ctx.fillStyle='rgba(255,255,255,.88)';
      ctx.beginPath();ctx.ellipse(x,y,51,18,0,0,Math.PI*2);ctx.fill();
      ctx.beginPath();ctx.arc(x-20,y-8,20,0,Math.PI*2);ctx.arc(x+9,y-16,27,0,Math.PI*2);ctx.fill();
    }
    ctx.restore();
  }
  function background(){
    const gradient=ctx.createLinearGradient(0,0,0,H);
    if(level?.decor==='mill'){gradient.addColorStop(0,'#81d6ec');gradient.addColorStop(1,'#f3fbba');}
    else if(level?.decor==='city'){gradient.addColorStop(0,'#a7d3dc');gradient.addColorStop(1,'#ffe7bb');}
    else if(level?.decor==='tunnel'){gradient.addColorStop(0,'#637e9a');gradient.addColorStop(1,'#cad1ba');}
    else if(level?.decor==='clock'){gradient.addColorStop(0,'#c4c9ea');gradient.addColorStop(1,'#f8edbf');}
    else if(level?.decor==='depot'){gradient.addColorStop(0,'#a9b4c7');gradient.addColorStop(1,'#e6d2ac');}
    else if(level?.decor==='mountain'){gradient.addColorStop(0,'#89a6c1');gradient.addColorStop(1,'#dfead7');}
    else if(level?.decor==='skyport'){gradient.addColorStop(0,'#77bcdb');gradient.addColorStop(1,'#f7e4bf');}
    else if(level?.decor==='crystal'){gradient.addColorStop(0,'#7788b0');gradient.addColorStop(1,'#d6bfdc');}
    else if(level?.decor==='mine'){gradient.addColorStop(0,'#788a91');gradient.addColorStop(1,'#d7bca6');}
    else if(level?.decor==='gardens'){gradient.addColorStop(0,'#a5bccc');gradient.addColorStop(1,'#e9e8bf');}
    else if(level?.decor==='foundry'){gradient.addColorStop(0,'#758296');gradient.addColorStop(1,'#e7b997');}
    else if(level?.decor==='gallery'){gradient.addColorStop(0,'#878cbe');gradient.addColorStop(1,'#f5dfbb');}
    else if(level?.decor==='heart'){gradient.addColorStop(0,'#576e9b');gradient.addColorStop(1,'#d8abc3');}
    else {gradient.addColorStop(0,'#8bdaf3');gradient.addColorStop(1,'#fbf8d5');}
    ctx.fillStyle=gradient;ctx.fillRect(0,0,W,H);
    ctx.fillStyle='#fff7ba';ctx.beginPath();ctx.arc(800-cam*.03,89,42,0,Math.PI*2);ctx.fill();
    clouds(cam);
    ctx.save();ctx.translate(-cam*.18,0);
    for(let x=-300;x<level.width+W;x+=210){
      ctx.fillStyle=x%420?'#a3e8a5':'#84d6a2';
      ctx.beginPath();ctx.ellipse(x,485,160,170,0,Math.PI,0);ctx.fill();
    }
    ctx.restore();
    ctx.save();ctx.translate(-cam*.5,0);
    for(let x=-100;x<level.width+W;x+=340){
      const y=380+(Math.abs(x/340)%2)*20;
      ctx.fillStyle='#569b75';ctx.fillRect(x+45,y-60,15,82);
      for(const a of [[40,-78,40],[79,-68,30],[62,-97,34]]){
        ctx.fillStyle='#45b984';ctx.beginPath();ctx.arc(x+a[0],y+a[1],a[2],0,Math.PI*2);ctx.fill();
      }
    }
    ctx.restore();
    if(level?.decor==='mountain'||level?.decor==='skyport'){
      ctx.save();ctx.translate(-cam*.16,0);
      for(let x=-380;x<level.width+W;x+=430){
        ctx.fillStyle=level.decor==='skyport'?'#a8cdd3':'#7f9aab';
        ctx.beginPath();ctx.moveTo(x,440);ctx.lineTo(x+230,155);ctx.lineTo(x+475,440);ctx.closePath();ctx.fill();
        ctx.fillStyle='#edf5ed';ctx.beginPath();ctx.moveTo(x+180,219);ctx.lineTo(x+230,155);ctx.lineTo(x+285,217);ctx.closePath();ctx.fill();
      }
      ctx.restore();
    }
    if(['gardens','foundry','gallery','heart'].includes(level?.decor)){
      ctx.save();ctx.translate(-cam*.24,0);
      for(let x=-350;x<level.width+W;x+=410){
        rounded(x+60,240,205,230,12,level.decor==='gardens'?'#8496a8':'#8f7c7f');
        rounded(x+120,145,83,125,7,level.decor==='gardens'?'#7b889b':'#786e7b');
        ctx.fillStyle=level.decor==='gardens'?'#dfe8b7':'#f7d3a5';
        ctx.beginPath();ctx.arc(x+165,300,30,0,Math.PI*2);ctx.fill();
      }
      ctx.restore();
    }
    if(level?.decor==='crystal'||level?.decor==='mine'){
      ctx.save();ctx.translate(-cam*.18,0);
      for(let x=-280;x<level.width+W;x+=370){
        ctx.fillStyle=level.decor==='crystal'?'#8d98c6':'#8d857e';
        ctx.beginPath();ctx.moveTo(x,450);ctx.lineTo(x+170,190);ctx.lineTo(x+360,450);ctx.closePath();ctx.fill();
        if(level.decor==='crystal'){
          ctx.fillStyle='#cadff4';ctx.beginPath();ctx.moveTo(x+172,190);ctx.lineTo(x+125,296);ctx.lineTo(x+220,288);ctx.closePath();ctx.fill();
        }
      }ctx.restore();
    }
    if(['city','tunnel','clock','depot'].includes(level?.decor)){
      ctx.save();ctx.translate(-cam*.28,0);
      for(let x=-300;x<level.width+W;x+=310){
        rounded(x+48,238,185,242,12,level.decor==='tunnel'?'#718592':'#a7a2a3');
        rounded(x+76,265,44,62,6,'#f4ddaa');
        rounded(x+145,265,44,62,6,'#f4ddaa');
        rounded(x+162,180,27,100,4,'#827e83');
        ctx.fillStyle='#e2e7df';ctx.beginPath();ctx.arc(x+175,180,31,0,Math.PI*2);ctx.fill();
        ctx.fillStyle='#8a9f9f';ctx.beginPath();ctx.arc(x+175,180,20,0,Math.PI*2);ctx.fill();
      }
      ctx.restore();
    }
  }
  function windmill(){
    const x=750-cam*.65,y=273;
    if(x<-160||x>W+160)return;
    ctx.save();ctx.translate(x,y);
    ctx.fillStyle='#f5e2a2';ctx.beginPath();ctx.moveTo(-40,130);ctx.lineTo(-26,-20);ctx.lineTo(31,-20);ctx.lineTo(44,130);ctx.fill();
    ctx.fillStyle='#a85a42';ctx.beginPath();ctx.moveTo(-43,-16);ctx.lineTo(0,-57);ctx.lineTo(39,-16);ctx.fill();
    rounded(-12,70,24,60,8,'#815846');
    ctx.translate(0,-13);ctx.rotate(Math.sin(frame/65)*0.05+frame*.002);
    for(let i=0;i<4;i++){ctx.save();ctx.rotate(i*Math.PI/2);ctx.fillStyle='#f4ebd2';ctx.fillRect(-9,-106,18,92);ctx.strokeStyle='#9b765c';ctx.lineWidth=3;ctx.strokeRect(-9,-106,18,92);ctx.restore();}
    ctx.fillStyle='#bc6f49';ctx.beginPath();ctx.arc(0,0,11,0,Math.PI*2);ctx.fill();ctx.restore();
  }
  function drawWorld(){
    background();if(level.decor==='mill')windmill();
    ctx.save();ctx.translate(-cam,0);
    for(const [i,g] of level.ground.entries()){
      rounded(g.x,g.y,g.w,H+90,0,'#b77b49');
      ctx.fillStyle='#76cf65';ctx.fillRect(g.x,g.y-9,g.w,14);
      ctx.fillStyle='#55ad51';ctx.fillRect(g.x,g.y,g.w,5);
      for(let x=g.x+17;x<g.x+g.w;x+=72){
        ctx.fillStyle='#d99f67';ctx.beginPath();ctx.arc(x,497+((x/70|0)%2)*24,6,0,Math.PI*2);ctx.fill();
        if(x%3<2){ctx.strokeStyle='#418a3d';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(x,g.y-10);ctx.lineTo(x,g.y-22);ctx.stroke();ctx.fillStyle=['#ffb9d5','#fff8b7','#e4b3fe'][((x/70)|0)%3];ctx.beginPath();ctx.arc(x,g.y-23,4,0,Math.PI*2);ctx.fill();}
      }
    }
    for(const b of level.platforms){rounded(b.x,b.y,b.w,b.h,7,'#b77a4d');rounded(b.x,b.y-5,b.w,11,6,'#72c760');}
    for(const belt of level.conveyors){
      rounded(belt.x,belt.y,belt.w,26,5,'#5b7078');
      for(let x=belt.x+18;x<belt.x+belt.w-12;x+=55)
        text(belt.speed>0?'›':'‹',x,belt.y+20,25,'#ffe8aa','center','800');
    }
    for(const crystal of level.crystals){
      rounded(crystal.x-13,GROUND-13,68,13,5,'#8193b1');
      ctx.fillStyle='#a8e9fa';ctx.beginPath();
      ctx.moveTo(crystal.x,GROUND-12);ctx.lineTo(crystal.x+20,GROUND-73);
      ctx.lineTo(crystal.x+43,GROUND-12);ctx.closePath();ctx.fill();
      ctx.fillStyle='#d8f9fd';ctx.beginPath();ctx.moveTo(crystal.x+20,GROUND-73);
      ctx.lineTo(crystal.x+20,GROUND-12);ctx.lineTo(crystal.x+43,GROUND-12);ctx.closePath();ctx.fill();
    }
    for(const gust of level.gusts){
      rounded(gust.x-13,GROUND-14,66,14,5,'#7da7b5');
      for(let i=0;i<3;i++){
        const yy=GROUND-24-((frame*1.2+i*31)%80);
        ctx.strokeStyle='rgba(220,247,252,.76)';ctx.lineWidth=3;
        ctx.beginPath();ctx.arc(gust.x+i*17,yy,9,Math.PI*.2,Math.PI*1.4);ctx.stroke();
      }
    }
    for(const dial of level.clockPads){
      rounded(dial.x-9,GROUND-15,59,13,5,'#708aa8');
      ctx.fillStyle='#fff6d4';ctx.beginPath();ctx.arc(dial.x+20,GROUND-36,25,0,Math.PI*2);ctx.fill();
      ctx.strokeStyle='#51728e';ctx.lineWidth=4;ctx.stroke();
      ctx.beginPath();ctx.moveTo(dial.x+20,GROUND-36);ctx.lineTo(dial.x+20,GROUND-52);
      ctx.moveTo(dial.x+20,GROUND-36);ctx.lineTo(dial.x+32,GROUND-36);ctx.stroke();
    }
    for(const vent of level.vents){
      rounded(vent.x-15,GROUND-16,65,16,5,'#758fa0');
      for(let i=0;i<3;i++){
        const yy=GROUND-20-((frame*1.5+i*27)%85);
        ctx.fillStyle='rgba(230,245,250,.6)';
        ctx.beginPath();ctx.arc(vent.x+i*16,yy,9,0,Math.PI*2);ctx.fill();
      }
    }
    for(const m of level.mushrooms){
      rounded(m.x+14,m.y-22,13,22,3,'#fff2df');
      ctx.fillStyle='#e58a7e';ctx.beginPath();ctx.ellipse(m.x+20,m.y-22,37,18,0,Math.PI,0);ctx.fill();
      text('↑',m.x+20,m.y-23,17,'#fff4d2','center');
    }
    for(const sw of level.switches){
      rounded(sw.x,sw.y-48,14,48,3,sw.on?'#4b9977':'#796a66');
      text(sw.on?'✓':'!',sw.x+7,sw.y-56,19,sw.on?'#2d8a6e':'#986342','center');
    }
    for(const [x,label] of level.sections) text(label,x+85,GROUND-175,16,'#315a6c','center','750');
    for(const cp of level.checkpoints){
      ctx.strokeStyle='#a9714b';ctx.lineWidth=5;ctx.beginPath();ctx.moveTo(cp,GROUND);ctx.lineTo(cp,GROUND-92);ctx.stroke();
      ctx.fillStyle=level.activeCheckpoint>=cp?'#69dfb1':'#fff1a7';
      ctx.beginPath();ctx.moveTo(cp,GROUND-91);ctx.lineTo(cp+55,GROUND-77);ctx.lineTo(cp,GROUND-61);ctx.fill();
    }
    for(const coin of level.coins){if(coin.got)continue;
      ctx.save();ctx.translate(coin.x,coin.y);ctx.scale(Math.max(.35,Math.abs(Math.cos(frame*.045+coin.id))),1);
      ctx.fillStyle='#f8bc32';ctx.beginPath();ctx.arc(0,0,13,0,Math.PI*2);ctx.fill();
      ctx.strokeStyle='#fff0a6';ctx.lineWidth=3;ctx.beginPath();ctx.arc(0,0,9,0,Math.PI*2);ctx.stroke();ctx.restore();
    }
    for(const f of level.fragments){if(f.got)continue;
      ctx.save();ctx.translate(f.x+12, f.y+4+Math.sin(frame*.06+f.id)*6);
      ctx.rotate(Math.PI/4);rounded(-13,-13,26,26,5,'#7afbe8');ctx.strokeStyle='#fff';ctx.lineWidth=3;ctx.strokeRect(-8,-8,16,16);ctx.restore();
    }
    for(const e of level.enemies){if(e.dead)continue;
      rounded(e.x,e.y,e.w,e.h,11,'#84588f');
      ctx.fillStyle='#fff';ctx.fillRect(e.x+6,e.y+8,7,7);ctx.fillRect(e.x+21,e.y+8,7,7);
      ctx.fillStyle='#2d254d';ctx.fillRect(e.x+9,e.y+10,3,3);ctx.fillRect(e.x+24,e.y+10,3,3);
    }
    if(level.boss&&level.boss.alive){
      const b=level.boss;
      rounded(b.x,b.y,b.w,b.h,19,b.hit>0?'#fba9a5':'#d5855e');
      ctx.strokeStyle='#703f38';ctx.lineWidth=5;
      for(let i=0;i<3;i++){ctx.beginPath();ctx.moveTo(b.x+11+i*26,b.y+48);ctx.lineTo(b.x+5+i*28,b.y+60);ctx.stroke();}
      ctx.fillStyle='#fff';ctx.fillRect(b.x+16,b.y+16,12,11);ctx.fillRect(b.x+45,b.y+16,12,11);
      ctx.fillStyle='#332e42';ctx.fillRect(b.x+20,b.y+19,5,5);ctx.fillRect(b.x+49,b.y+19,5,5);
      text(b.name.toUpperCase(),b.x+36,b.y-35,13,'#733e2c','center');
      for(let i=0;i<b.hp;i++)text('♥',b.x+14+i*20,b.y-16,16,'#ec5f5f');
    }
    // Exit remains visible, but needs the three parts and a defeated boss.
    const open=C.canExit(level.fragments.filter(f=>f.got).length,!!level.boss,!level.boss||!level.boss.alive);
    ctx.fillStyle=open?'#a8e7ba':'#bfc9c9';ctx.fillRect(level.gate,GROUND-93,54,93);
    rounded(level.gate+8,GROUND-81,38,78,7,open?'#61be88':'#84929c');
    text(open?'ВЫХОД':'ЗАМОК',level.gate+27,GROUND-106,15,'#374d56','center');
    for(let i=0;i<3;i++)text(i<level.fragments.filter(f=>f.got).length?'◆':'◇',level.gate+i*15+7,GROUND-54,14,'#f9efc1');
    if(p.inv<=0||Math.floor(frame/5)%2===0){
      // NEUTRAL TECHNICAL MARKER. Official fox artwork must be inserted unchanged later.
      rounded(p.x,p.y,p.w,p.h,10,'#315d90');
      rounded(p.x+5,p.y+6,p.w-10,p.h-14,6,'#75c2df');
      text('Р',p.x+p.w/2,p.y+30,24,'#193852','center','800');
    }
    ctx.restore();
    if(toastTime>0){
      rounded(165,H-92,W-330,54,15,'rgba(33,64,78,.83)');
      text(toast,W/2,H-58,Math.min(17,Math.floor(740/Math.max(toast.length,1))), '#fff','center','650');
    }
  }
  function overlay(title,lines,action){
    ctx.fillStyle='rgba(17,49,65,.72)';ctx.fillRect(0,0,W,H);
    rounded(180,107,600,320,26,'rgba(249,253,232,.98)');
    ctx.fillStyle='#4da782';ctx.fillRect(180,107,600,11);
    text(title,480,183,33,'#234c5a','center','800');
    let y=232;
    for(const line of lines){text(line,480,y,18,'#345b5a','center','600');y+=34;}
    rounded(285,355,390,51,14,'#3a9a77');
    text(action,480,387,20,'#fff','center');
  }
  function paint(){
    ctx.clearRect(0,0,W,H);
    if(mode==='title'){
      const gradient=ctx.createLinearGradient(0,0,0,H);gradient.addColorStop(0,'#7ed4ec');gradient.addColorStop(1,'#d7ef9d');ctx.fillStyle=gradient;ctx.fillRect(0,0,W,H);
      clouds(frame*2);ctx.fillStyle='#86bf6b';ctx.beginPath();ctx.ellipse(480,555,740,230,0,Math.PI,0);ctx.fill();
      rounded(115,105,730,345,26,'rgba(252,255,238,.91)');
      text('RIKITU',480,190,69,'#318168','center','900');
      text('ЗАВОДНЫЕ ТРОПЫ',480,250,35,'#284b5f','center','850');
      text('Игровой прототип • Четыре мира, шестнадцать уровней',480,293,18,'#5d7d79','center','600');
      rounded(268,341,424,55,15,'#338f70');text(startupSave?'ENTER — ПРОДОЛЖИТЬ':'ENTER / ПРОБЕЛ — НАЧАТЬ',480,378,20,'#fff','center');
      text('Герой временно обозначен маркером «Р» — дизайн Рикиту не изменяется.',480,486,15,'#345e65','center','500');
    } else {
      drawWorld();
      if(mode==='paused')overlay('ПАУЗА',['Игра остановлена.','P — продолжить • R — начать заново'],'P — ПРОДОЛЖИТЬ');
      else if(mode==='between')overlay('УРОВЕНЬ ПРОЙДЕН!',[level.endText,`Время: ${formatClock(stageSeconds)} · попаданий: ${stageHits} · монет: ${stageCoins}/90`,`Следующий уровень: ${chapterInfo[levelIndex+1].name}`],`ENTER — УРОВЕНЬ ${levelIndex+2}`);
      else if(mode==='shop'){
        const offer=C.nextHeartPurchase(shopStage,purchasedHearts,allCoins);
        const n=C.unlockedUpgrades(shopStage);
        const lines=[`Монеты: ${allCoins} · Сердца: ${lifeCapacity()}/7`,
          `Уровень ${levelIndex+1}: ${formatClock(stageSeconds)} · попаданий: ${stageHits} · монет: ${stageCoins}/90`,
          `Открыто улучшений: ${n} из 4 · Куплено: ${purchasedHearts}`];
        if(offer.price){
          lines.push(`Следующее сердечко: ${offer.price} монет`);
          lines.push(offer.available?'B — КУПИТЬ СЕРДЕЧКО':'Монет пока не хватает — покупку можно отложить!');
        }else if(lifeCapacity()===7)lines.push('Получены все семь сердечек!');
        else lines.push('Следующее сердечко станет доступно в следующем мире.');
        overlay('МОКИ — ЛАВКА СЕРДЕЦ',lines,'ENTER — ПРОДОЛЖИТЬ');
        text('♥',745,180,33,'#d75f6d','center');
      }
      else if(mode==='victory')overlay('СЕРДЦЕ ДОРОГ СПАСЕНО!',['Хроном остановил механизм, чтобы не допустить катастрофы.','Рикиту собрал 48 фрагментов и установил регулятор Моториуса.','Часы снова идут: 07:13. Королевство объединено!',`Последний уровень: ${formatClock(stageSeconds)} · попаданий: ${stageHits}`],'ENTER — НАЧАТЬ НОВУЮ ИГРУ');
    }
  }
  function togglePause(){if(mode==='playing'){mode='paused'}else if(mode==='paused'){mode='playing'}}
  function buyUpgrade(){
    if(mode!=='shop')return false;
    const purchase=C.buyHeart(shopStage,purchasedHearts,allCoins);
    if(!purchase.success)return false;
    purchasedHearts=purchase.purchasedCount;allCoins=purchase.coins;
    if(p)p.hearts=Math.min(lifeCapacity(),p.hearts+1);
    announce(`Спасибо, Моки! Теперь ${lifeCapacity()} сердечка здоровья!`,2.3);
    updateHud();persist();return true;
  }
  function exportStageResults(){
    if(!stageRecords.length)return false;
    const rows=[['Уровень','Название','Тип','Секунды','Попадания','Монеты из 90','Цель от, с','Цель до, с','Сравнение']];
    for(const r of [...stageRecords].sort((a,b)=>a.stage-b.stage)){
      const bossStage=r.stage%2===0;
      const min=bossStage?480:360,max=bossStage?600:480;
      const verdict=r.seconds<min?'Ниже цели':r.seconds>max?'Выше цели':'В пределах цели';
      rows.push([r.stage,chapterInfo[r.stage-1].name,bossStage?'С мини-боссом':'Обычный',
        r.seconds,r.hits,r.coins,min,max,verdict]);
    }
    const quote=v=>'"'+String(v).replace(/"/g,'""')+'"';
    const csv='\uFEFF'+rows.map(row=>row.map(quote).join(';')).join('\r\n')+'\r\n';
    const url=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8'}));
    const a=document.createElement('a');a.href=url;a.download='RIKITU_stage_results.csv';
    document.body.appendChild(a);a.click();a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
    return true;
  }
  function resetCampaign(){try{localStorage.removeItem(SAVE_KEY)}catch(e){}startupSave=null;allCoins=0;purchasedHearts=0;shopStage=0;stageRecords=[];newLevel(0);}
  function act(){
    if(mode==='title'){if(!resume())resetCampaign();}
    else if(mode==='between')newLevel(levelIndex+1);
    else if(mode==='shop'){mode=levelIndex===chapterInfo.length-1?'victory':'between';persist();}
    else if(mode==='victory')resetCampaign();
    else if(mode==='paused')togglePause();
  }
  const eventButtons=new Set(['ArrowLeft','ArrowRight','ArrowUp','Space','ShiftLeft','KeyA','KeyD','KeyW','Enter','KeyP','Escape','KeyR','KeyB']);
  addEventListener('keydown',e=>{
    if(eventButtons.has(e.code))e.preventDefault();
    if(!keys.has(e.code)){
      if(e.code==='Enter'||(e.code==='Space'&&mode!=='playing'))act();
      if(e.code==='KeyP'||e.code==='Escape')togglePause();
      if(e.code==='KeyB'&&mode==='shop')buyUpgrade();
      if(e.code==='KeyR')resetCampaign();
    }
    keys.add(e.code);
  });
  addEventListener('keyup',e=>keys.delete(e.code));
  addEventListener('blur',()=>{keys.clear();touch.left=false;touch.right=false;touch.jump=false;if(mode==='playing')mode='paused';});
  document.querySelector('#start').addEventListener('click',act);
  document.querySelector('#pause').addEventListener('click',togglePause);
  document.querySelector('#restart').addEventListener('click',resetCampaign);
  document.querySelector('#export-results').addEventListener('click',exportStageResults);
  document.querySelector('#buy-heart').addEventListener('click',buyUpgrade);
  document.querySelectorAll('[data-touch]').forEach(btn=>{
    const dir=btn.getAttribute('data-touch');
    btn.addEventListener('pointerdown',e=>{e.preventDefault();touch[dir]=true;btn.setPointerCapture(e.pointerId)});
    const off=e=>{e.preventDefault();touch[dir]=false};
    btn.addEventListener('pointerup',off);btn.addEventListener('pointercancel',off);
  });
  function loop(ts){
    const dt=Math.min(.10,(ts-lastTime)/1000||fixed);lastTime=ts;acc+=dt;
    let count=0;while(acc>=fixed&&count<6){update(fixed);acc-=fixed;count++;}
    paint();requestAnimationFrame(loop);
  }
  // Read-only health probe for browser integration tests.
  window.RIKITU_TEST_STATE=()=>({mode,level:levelIndex+1,playerX:p?.x,playerY:p?.y,fragments:level?.fragments.filter(x=>x.got).length,coins:stageCoins,bossHP:level?.boss?.hp??null,bossX:level?.boss?.x??null,bossY:level?.boss?.y??null,checkpoint:level?.activeCheckpoint,checkpointCount:level?.checkpointCount,hearts:p?.hearts,maxHearts:lifeCapacity(),wallet:allCoins,bought:purchasedHearts,shopStage,hasSave:!!startupSave,elapsedSeconds:Math.round(stageSeconds*100)/100,hitCount:stageHits,stageRecords:stageRecords.map(x=>({...x}))});
  window.RIKITU_TEST_SHOP=(coins=0,bought=purchasedHearts,stage=shopStage)=>C.nextHeartPurchase(stage,bought,coins);
  window.RIKITU_TEST_COLLECT_COINS=(count=90)=>{
    if(!level||!Number.isInteger(count)||count<0)return false;
    for(const coin of level.coins){
      if(count===0)break;
      if(!coin.got){coin.got=true;stageCoins++;allCoins++;count--;}
    }
    updateHud();return true;
  };
  window.RIKITU_TEST_LEVELS=()=>chapterInfo.map(d=>({width:d.width,fragments:d.fragments,coins:d.coins.length,checkpoints:d.checkpoints,sections:d.sections,gate:d.gate,boss:d.boss,mushrooms:d.mushrooms||[],switches:d.switches||[],conveyors:d.conveyors||[],vents:d.vents||[],clockPads:d.clockPads||[],gusts:d.gusts||[],crystals:d.crystals||[]}));
  window.RIKITU_TEST_MOVE=(x,y)=>{if(p&&level){p.x=clamp(x,0,level.width-p.w);p.y=y;p.vy=0;p.inv=2;}};
  updateHud();requestAnimationFrame(loop);
})();
