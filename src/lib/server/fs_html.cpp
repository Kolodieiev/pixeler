#include <stdint.h>

static constexpr char HEAD_HTML[] =
    R"rawliteral(<!DOCTYPE html><html lang="uk"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>)rawliteral";

static constexpr char STYLE_HTML[] = R"rawliteral(</title><style>
:root{color-scheme:dark;--bg:#0d1117;--surface:#161b22;--fg:#e6edf3;--muted:#8b949e;--border:#30363d;--hover:#161b22;--accent:#58a6ff;--ok:#3fb950;--err:#f85149}
*{box-sizing:border-box}
body{margin:0;padding:14px;background:var(--bg);color:var(--fg);font:15px/1.45 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}
.wrap{max-width:920px;margin:0 auto}
.hdr{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:8px;padding-bottom:10px;border-bottom:1px solid var(--border);margin-bottom:12px}
.crumbs{font-size:16px;font-weight:600;word-break:break-all}
.crumbs a{color:var(--accent);text-decoration:none}
.crumbs a:hover{text-decoration:underline}
.meta{display:flex;align-items:center;gap:10px;color:var(--muted);font-size:13px}
.btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:6px 12px;border:1px solid var(--border);border-radius:6px;background:var(--surface);color:var(--fg);text-decoration:none;font:inherit;font-size:13px;cursor:pointer}
.btn:hover:not(:disabled){border-color:var(--accent);color:var(--accent)}
.btn:disabled{opacity:.45;cursor:not-allowed}
.btn-pri{background:var(--accent);border-color:var(--accent);color:#0d1117;font-weight:600}
.btn-pri:hover:not(:disabled){opacity:.9;color:#0d1117}
.btn-err{border-color:var(--err);color:var(--err)}
.btn-err:hover:not(:disabled){background:rgba(248,81,73,.12);border-color:var(--err);color:var(--err)}
table{width:100%;border-collapse:collapse}
th,td{padding:8px 6px;border-bottom:1px solid var(--border);text-align:left}
th{color:var(--muted);font-size:13px;font-weight:600}
tr:hover td{background:var(--hover)}
.ic{width:28px;text-align:center}
.nm{word-break:break-word}
.nm a{display:block;color:var(--fg);text-decoration:none;word-break:break-word}
.nm a:hover{color:var(--accent);text-decoration:underline}
.sz{width:1%;text-align:right;color:var(--muted);white-space:nowrap;font-size:13px;padding-left:10px}
.rm{width:34px;text-align:center}
.btn-rm{display:inline-flex;align-items:center;justify-content:center;width:26px;height:26px;padding:0;border:1px solid var(--border);border-radius:6px;background:transparent;color:var(--muted);font:inherit;font-size:13px;cursor:pointer}
.btn-rm:hover:not(:disabled){border-color:var(--err);color:var(--err);background:rgba(248,81,73,.12)}
.btn-rm:disabled{opacity:.35;cursor:not-allowed}
.empty{padding:24px 6px;text-align:center;color:var(--muted)}
.pg{display:flex;flex-wrap:wrap;align-items:center;justify-content:center;gap:6px;margin:12px 0}
.pg a,.pg span{padding:5px 10px;border:1px solid var(--border);border-radius:6px;text-decoration:none;color:var(--fg);font-size:14px}
.pg a:hover{background:var(--hover);border-color:var(--accent)}
.pg .cur{background:var(--accent);border-color:var(--accent);color:#0d1117;font-weight:600}
.pg .dis{opacity:.4;pointer-events:none}
.pg .dots{border:none;padding:5px 4px;color:var(--muted)}
.card{background:var(--surface);border:1px solid var(--border);border-radius:8px;padding:18px}
.drop{display:block;border:2px dashed var(--border);border-radius:8px;padding:28px 16px;text-align:center;cursor:pointer;transition:border-color .15s,background .15s}
.drop:hover,.drop.over{border-color:var(--accent);background:rgba(88,166,255,.06)}
.drop input{display:none}
.drop-ic{font-size:32px;line-height:1;margin-bottom:8px}
.drop-sub{color:var(--muted);font-size:13px;margin-top:4px}
.acts{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:8px;margin-top:12px}
.acts-g{display:flex;flex-wrap:wrap;gap:8px}
.bar{height:8px;background:var(--bg);border:1px solid var(--border);border-radius:999px;overflow:hidden;margin-top:12px}
.bar>div{height:100%;width:0;background:var(--accent);transition:width .12s}
.bar.ok>div{background:var(--ok)}
.bar.err>div{background:var(--err)}
.st{display:flex;justify-content:space-between;gap:8px;margin-top:6px;font-size:13px;color:var(--muted);word-break:break-word}
.q{margin-top:14px}
.tag{font-size:12px;white-space:nowrap}
.t-wait{color:var(--muted)}
.t-up{color:var(--accent)}
.t-ok{color:var(--ok)}
.t-err{color:var(--err)}
.nf{max-width:480px;margin:48px auto;text-align:center;padding:28px 20px}
.nf-ic{font-size:40px;line-height:1;margin-bottom:8px}
.nf-code{font-size:48px;font-weight:700;color:var(--accent);line-height:1.1;margin:0}
.nf-msg{font-size:16px;margin:10px 0 6px}
.nf-sub{color:var(--muted);font-size:13px;margin:0 0 18px;word-break:break-all}
.nf-acts{display:flex;justify-content:center;gap:10px;flex-wrap:wrap}
</style></head><body><div class="wrap">)rawliteral";

static constexpr char FOOT_HTML[] = "</div></body></html>";

static constexpr char RECEIVE_TITLE_STR[] = "Надіслати файл";
static constexpr char SEND_TITLE_STR[] = "Список файлів";
static constexpr char NOT_FOUND_TITLE_STR[] = "404 — Не знайдено";

static constexpr char NOT_FOUND_BODY_START[] =
    R"rawliteral(<div class="card nf"><div class="nf-ic">🔍</div><h1 class="nf-code">404</h1><p class="nf-msg">Файл або сторінку не знайдено</p><p class="nf-sub">)rawliteral";

static constexpr char NOT_FOUND_BODY_END[] =
    R"rawliteral(</p><div class="nf-acts"><a class="btn btn-pri" href="/">🏠 На головну</a><a class="btn" href="javascript:history.back()">⬅️ Назад</a></div></div>)rawliteral";

static constexpr char RECEIVE_BODY_HTML[] = R"rawliteral(<div class="hdr"><div class="crumbs">📤 Завантаження на сервер</div><div class="meta" id="meta">Оберіть файли для передачі</div></div>
<form class="card" id="f" method="POST" action="/upload" enctype="multipart/form-data">
<label class="drop" id="drop"><input type="file" id="inp" name="file" multiple><div class="drop-ic">📥</div><div><strong>Перетягніть файли сюди</strong> або натисніть для вибору</div><div class="drop-sub">Підтримується один або кілька файлів</div></label>
<div class="acts"><div class="acts-g"><button type="submit" class="btn btn-pri" id="up" disabled>⬆️ Завантажити</button><button type="button" class="btn" id="clr" disabled>🗑️ Очистити</button></div><button type="button" class="btn btn-err" id="can" style="display:none">⛔ Скасувати</button></div>
<div id="pbox" style="display:none"><div class="bar" id="bar"><div id="fill"></div></div><div class="st"><span id="stL"></span><span id="stR"></span></div></div>
</form>
<div class="q" id="qbox" style="display:none"><table><thead><tr><th class="ic"></th><th>Файл</th><th class="sz">Розмір</th><th class="sz">Статус</th><th class="rm"></th></tr></thead><tbody id="tb"></tbody></table></div>
<script>
(()=>{
const $=id=>document.getElementById(id);
const drop=$('drop'),inp=$('inp'),up=$('up'),clr=$('clr'),can=$('can');
const pbox=$('pbox'),bar=$('bar'),fill=$('fill'),stL=$('stL'),stR=$('stR');
const qbox=$('qbox'),tb=$('tb'),meta=$('meta');
let queue=[],busy=false,aborted=false,xhr=null;
const fmt=b=>{
  if(b<1024)return b+' B';
  if(b<1048576)return(b/1024).toFixed(1)+' KB';
  if(b<1073741824)return(b/1048576).toFixed(2)+' MB';
  return(b/1073741824).toFixed(2)+' GB';
};
const setRow=(i,txt,cls)=>{
  const it=queue[i];
  if(it){it.txt=txt;it.cls=cls;}
  const c=tb.rows[i]&&tb.rows[i].cells[3];
  if(!c)return;
  c.textContent=txt;
  c.className='sz tag '+cls;
};
const setRmDisabled=dis=>{
  tb.querySelectorAll('.btn-rm').forEach(b=>b.disabled=dis);
};
const updateControls=()=>{
  const hasPending=queue.some(it=>!it.done);
  up.disabled=busy||!hasPending;
  clr.disabled=busy||!queue.length;
};
const render=()=>{
  tb.textContent='';
  let total=0;
  queue.forEach((it,idx)=>{
    total+=it.file.size;
    const tr=tb.insertRow();
    const c0=tr.insertCell();c0.className='ic';c0.textContent='📄';
    const c1=tr.insertCell();c1.className='nm';c1.textContent=it.file.name;
    const c2=tr.insertCell();c2.className='sz';c2.textContent=fmt(it.file.size);
    const c3=tr.insertCell();c3.className='sz tag '+it.cls;c3.textContent=it.txt;
    const c4=tr.insertCell();c4.className='rm';
    const btn=document.createElement('button');
    btn.type='button';
    btn.className='btn-rm';
    btn.title='Видалити зі списку';
    btn.textContent='✕';
    btn.disabled=busy;
    btn.onclick=()=>{
      if(busy)return;
      queue.splice(idx,1);
      if(!queue.length)pbox.style.display='none';
      render();
    };
    c4.appendChild(btn);
  });
  qbox.style.display=queue.length?'':'none';
  updateControls();
  meta.textContent=queue.length?queue.length+' шт. ('+fmt(total)+')':'Оберіть файли для передачі';
};
const addFiles=list=>{
  if(busy||!list||!list.length)return;
  if(queue.length&&queue.every(it=>it.done))queue=[];
  for(const f of list)queue.push({file:f,txt:'Очікує',cls:'t-wait',done:false});
  inp.value='';
  pbox.style.display='none';
  bar.className='bar';
  fill.style.width='0%';
  render();
};
inp.addEventListener('change',()=>addFiles(inp.files));
['dragenter','dragover'].forEach(e=>drop.addEventListener(e,ev=>{ev.preventDefault();if(!busy)drop.classList.add('over');}));
['dragleave','drop'].forEach(e=>drop.addEventListener(e,ev=>{ev.preventDefault();drop.classList.remove('over');}));
drop.addEventListener('drop',ev=>addFiles(ev.dataTransfer.files));
clr.addEventListener('click',()=>{
  if(busy)return;
  queue=[];inp.value='';pbox.style.display='none';render();
});
can.addEventListener('click',()=>{
  aborted=true;
  if(xhr)xhr.abort();
});
const sendOne=(file,idx)=>new Promise(res=>{
  const fd=new FormData();
  fd.append('file',file,file.name);
  xhr=new XMLHttpRequest();
  xhr.open('POST','/upload?xhr=1',true);
  xhr.upload.onprogress=e=>{
    if(!e.lengthComputable||e.total===0)return;
    const p=Math.min(100,Math.round(e.loaded*100/e.total));
    fill.style.width=p+'%';
    stR.textContent=fmt(e.loaded)+' / '+fmt(e.total)+' ('+p+'%)';
    setRow(idx,p+'%','t-up');
  };
  xhr.onload=()=>{
    if(xhr.status>=200&&xhr.status<300){queue[idx].done=true;setRow(idx,'✅ Готово','t-ok');res('ok');}
    else{setRow(idx,'❌ Помилка '+xhr.status,'t-err');res('err');}
  };
  xhr.onerror=()=>{setRow(idx,'❌ Помилка мережі','t-err');res('err');};
  xhr.onabort=()=>{setRow(idx,'⛔ Скасовано','t-err');res('abort');};
  xhr.send(fd);
});
$('f').addEventListener('submit',async ev=>{
  ev.preventDefault();
  const pending=queue.filter(it=>!it.done).length;
  if(busy||!pending)return;
  busy=true;aborted=false;
  up.disabled=true;clr.disabled=true;inp.disabled=true;
  setRmDisabled(true);
  can.style.display='';pbox.style.display='';
  for(let i=0;i<queue.length;i++)if(!queue[i].done)setRow(i,'Очікує','t-wait');
  let step=0,fail=0;
  for(let i=0;i<queue.length;i++){
    if(queue[i].done)continue;
    if(aborted){setRow(i,'⛔ Скасовано','t-err');continue;}
    step++;
    bar.className='bar';fill.style.width='0%';
    stL.textContent='Завантаження '+step+'/'+pending+': '+queue[i].file.name;
    stR.textContent='0%';
    setRow(i,'0%','t-up');
    const r=await sendOne(queue[i].file,i);
    xhr=null;
    if(r!=='ok')fail++;
  }
  const okTotal=queue.filter(it=>it.done).length;
  busy=false;inp.disabled=false;can.style.display='none';
  setRmDisabled(false);
  updateControls();
  if(aborted){
    bar.className='bar err';
    stL.textContent='Завантаження скасовано ('+okTotal+'/'+queue.length+')';
  }else if(fail){
    bar.className='bar err';
    stL.textContent='Завершено з помилками: '+okTotal+' готово, '+fail+' помилок';
  }else{
    bar.className='bar ok';fill.style.width='100%';
    stL.textContent='Усі файли успішно завантажено ('+okTotal+'/'+queue.length+')';
    stR.textContent='100%';
  }
});
})();
</script>)rawliteral";
