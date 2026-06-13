#include <WiFi.h>
#include <ESPmDNS.h>
#include <FastLED.h>

#define NUM_LEDS   25
#define DATA_PIN   4
#define BRIGHTNESS 128

const char *ssid = "Net4me";
const char *password = "brousovaubych";
NetworkServer server(80);
CRGB leds[NUM_LEDS];

void sendOK(NetworkClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Length: 0");
  client.println("Connection: close");
  client.println();
}

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected: " + WiFi.localIP().toString());
  server.begin();
  MDNS.begin("lantern");
}

void loop()
{
  NetworkClient client = server.accept();
  if (!client) return;

  String currentLine = "";
  unsigned long timeout = millis();

  while (client.connected() && millis() - timeout < 800) {
    if (!client.available()) continue;
    timeout = millis();
    char c = client.read();

    if (c == '\n') {
      if (currentLine.length() == 0) {
        // serve HTML only for root
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
        client.print(R"rawhtml(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Lantern</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;font-family:sans-serif;display:flex;justify-content:center;padding:1.5rem 1rem}
.wrap{width:100%;max-width:380px;display:flex;flex-direction:column;gap:1.25rem}
.card{background:#1e1e1e;border:1px solid #2e2e2e;border-radius:14px;padding:1rem 1.25rem}
.row{display:flex;align-items:center;justify-content:space-between}
.sec-title{font-size:13px;font-weight:500;color:#888;margin-bottom:.75rem}
.toggle-track{width:52px;height:28px;border-radius:14px;background:#333;position:relative;cursor:pointer;border:none;outline:none;flex-shrink:0;transition:background .2s}
.toggle-track.on{background:#2a9d5c}
.toggle-thumb{width:22px;height:22px;border-radius:50%;background:#fff;position:absolute;top:3px;left:3px;transition:left .2s;box-shadow:0 1px 3px rgba(0,0,0,.4)}
.toggle-track.on .toggle-thumb{left:27px}
.device-name{font-size:16px;font-weight:500;color:#eee}
.slider-row{display:flex;align-items:center;gap:10px}
.slider-row input[type=range]{flex:1;accent-color:#fff;cursor:pointer}
.slider-val{font-size:13px;color:#888;min-width:30px;text-align:right}
.wheel-wrap{display:flex;justify-content:center;padding:.5rem 0}
canvas{border-radius:50%;cursor:crosshair;display:block;touch-action:none}
.preview{width:100%;height:44px;border-radius:10px;border:1px solid #2e2e2e;margin-top:.75rem;transition:background .1s}
.presets{display:grid;grid-template-columns:repeat(5,1fr);gap:8px;margin-top:.25rem}
.preset-btn{border:1px solid #2e2e2e;border-radius:10px;padding:8px 4px;cursor:pointer;background:#1e1e1e;display:flex;flex-direction:column;align-items:center;gap:5px}
.preset-btn:active{transform:scale(.95)}
.preset-btn.active{border:2px solid #aaa}
.preset-dot{width:16px;height:16px;border-radius:50%}
.preset-label{font-size:10px;color:#888;text-align:center;line-height:1.3}
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <div class="row">
      <span class="device-name">Lantern</span>
      <button class="toggle-track" id="toggle" onclick="togglePower()">
        <div class="toggle-thumb"></div>
      </button>
    </div>
  </div>
  <div class="card">
    <div class="sec-title">Brightness</div>
    <div class="slider-row">
      <span style="font-size:13px;color:#888">&#9788;</span>
      <input type="range" min="0" max="255" value="128" id="brightness" oninput="onBrightness(this.value)">
      <span style="font-size:13px;color:#888;min-width:30px;text-align:right" id="bval">50%</span>
    </div>
  </div>
  <div class="card">
    <div class="sec-title">Color</div>
    <div class="wheel-wrap">
      <canvas id="wheel" width="220" height="220"></canvas>
    </div>
    <div class="preview" id="preview" style="background:rgb(255,255,255)"></div>
  </div>
  <div class="card">
    <div class="sec-title">Temperature</div>
    <div class="presets" id="presets"></div>
  </div>
</div>
<script>
const PRESETS=[
  {label:'Candle',K:'1800K',r:255,g:147,b:41},
  {label:'Warm',K:'2700K',r:255,g:197,b:143},
  {label:'Neutral',K:'4000K',r:255,g:228,b:205},
  {label:'Cool',K:'5500K',r:220,g:229,b:255},
  {label:'Day',K:'6500K',r:201,g:218,b:255}
];
let powered=false,dragging=false;
let pendingColor=null,colorTimer=null,sending=false;

function togglePower(){
  powered=!powered;
  document.getElementById('toggle').classList.toggle('on',powered);
  fetch(powered?'/on':'/off').catch(()=>{});
}

let bTimer=null;
function onBrightness(v){
  document.getElementById('bval').textContent=Math.round(v/255*100)+'%';
  clearTimeout(bTimer);
  bTimer=setTimeout(()=>fetch('/brightness?v='+v).catch(()=>{}),80);
}

function sendColor(r,g,b){
  if(sending){pendingColor={r,g,b};return;}
  sending=true;
  fetch('/color?r='+r+'&g='+g+'&b='+b)
    .catch(()=>{})
    .finally(()=>{
      sending=false;
      if(pendingColor){
        const c=pendingColor;pendingColor=null;
        sendColor(c.r,c.g,c.b);
      }
    });
}

function setPreview(r,g,b){
  document.getElementById('preview').style.background='rgb('+r+','+g+','+b+')';
}

function buildPresets(){
  const c=document.getElementById('presets');
  PRESETS.forEach(p=>{
    const b=document.createElement('button');
    b.className='preset-btn';
    b.innerHTML='<div class="preset-dot" style="background:rgb('+p.r+','+p.g+','+p.b+')"></div><span class="preset-label">'+p.label+'<br>'+p.K+'</span>';
    b.onclick=()=>{
      setPreview(p.r,p.g,p.b);
      document.querySelectorAll('.preset-btn').forEach(x=>x.classList.remove('active'));
      b.classList.add('active');
      sendColor(p.r,p.g,p.b);
    };
    c.appendChild(b);
  });
}

function drawWheel(){
  const cv=document.getElementById('wheel');
  const ctx=cv.getContext('2d');
  const cx=110,cy=110,r=106;
  const conic=ctx.createConicGradient(-Math.PI/2,cx,cy);
  for(let i=0;i<=360;i++) conic.addColorStop(i/360,'hsl('+i+',100%,50%)');
  ctx.beginPath();ctx.arc(cx,cy,r,0,Math.PI*2);
  ctx.fillStyle=conic;ctx.fill();
  const radial=ctx.createRadialGradient(cx,cy,0,cx,cy,r);
  radial.addColorStop(0,'rgba(255,255,255,1)');
  radial.addColorStop(1,'rgba(255,255,255,0)');
  ctx.beginPath();ctx.arc(cx,cy,r,0,Math.PI*2);
  ctx.fillStyle=radial;ctx.fill();
}

function pickColor(e){
  const cv=document.getElementById('wheel');
  const rect=cv.getBoundingClientRect();
  const touch=e.touches?e.touches[0]:e;
  const x=touch.clientX-rect.left,y=touch.clientY-rect.top;
  const scale=cv.width/rect.width;
  const d=cv.getContext('2d').getImageData(Math.round(x*scale),Math.round(y*scale),1,1).data;
  if(d[3]<10)return;
  setPreview(d[0],d[1],d[2]);
  document.querySelectorAll('.preset-btn').forEach(x=>x.classList.remove('active'));
  sendColor(d[0],d[1],d[2]);
}

const cv=document.getElementById('wheel');
cv.addEventListener('mousedown',e=>{dragging=true;pickColor(e);});
cv.addEventListener('mousemove',e=>{if(dragging)pickColor(e);});
document.addEventListener('mouseup',()=>dragging=false);
cv.addEventListener('touchstart',e=>{e.preventDefault();pickColor(e);},{passive:false});
cv.addEventListener('touchmove',e=>{e.preventDefault();pickColor(e);},{passive:false});
drawWheel();buildPresets();
</script>
</body>
</html>)rawhtml");
        break;
      }
      else currentLine = "";
    }
    else if (c != '\r') {
      currentLine += c;

      if (currentLine.endsWith("/on")) {
        fill_solid(leds, NUM_LEDS, CRGB::White);
        FastLED.show();
        sendOK(client); break;
      }
      if (currentLine.endsWith("/off")) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        sendOK(client); break;
      }
      if (currentLine.indexOf("/brightness?v=") >= 0 && currentLine.endsWith(" HTTP/1.1")) {
        int idx = currentLine.indexOf("?v=") + 3;
        int val = currentLine.substring(idx).toInt();
        FastLED.setBrightness(val);
        FastLED.show();
        sendOK(client); break;
      }
      if (currentLine.indexOf("/color?r=") >= 0 && currentLine.endsWith(" HTTP/1.1")) {
        int ampG = currentLine.indexOf("&g=");
        int ampB = currentLine.indexOf("&b=");
        int spH  = currentLine.indexOf(" HTTP/1.1");
        int r = currentLine.substring(currentLine.indexOf("r=") + 2, ampG).toInt();
        int g = currentLine.substring(ampG + 3, ampB).toInt();
        int b = currentLine.substring(ampB + 3, spH).toInt();
        fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
        FastLED.show();
        sendOK(client); break;
      }
    }
  }

  client.stop();
}
