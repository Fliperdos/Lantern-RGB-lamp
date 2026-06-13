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

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to "); Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  MDNS.begin("lantern");
}

void loop()
{
  NetworkClient client = server.accept();

  if (client)
  {
    Serial.println("New Client.");
    String currentLine = "";
    unsigned long timeout = millis();

    while (client.connected() && millis() - timeout < 800)
    {
      if (client.available())
      {
        timeout = millis();
        char c = client.read();

        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
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
.label{font-size:13px;color:#888}
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
.preset-btn{border:1px solid #2e2e2e;border-radius:10px;padding:8px 4px;cursor:pointer;background:#1e1e1e;display:flex;flex-direction:column;align-items:center;gap:5px;transition:border-color .15s}
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
      <span class="label">&#9788;</span>
      <input type="range" min="0" max="255" value="128" id="brightness" oninput="onBrightness(this.value)">
      <span class="slider-val" id="bval">50%</span>
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
let powered=false,curR=255,curG=255,curB=255,dragging=false;
function togglePower(){
  powered=!powered;
  document.getElementById('toggle').classList.toggle('on',powered);
  fetch(powered?'/on':'/off').catch(()=>{});
}
function onBrightness(v){
  document.getElementById('bval').textContent=Math.round(v/255*100)+'%';
  fetch('/brightness?v='+v).catch(()=>{});
}
function sendColor(r,g,b){
  fetch('/color?r='+r+'&g='+g+'&b='+b).catch(()=>{});
}
function setPreview(r,g,b){
  document.getElementById('preview').style.background='rgb('+r+','+g+','+b+')';
}
function buildPresets(){
  const c=document.getElementById('presets');
  PRESETS.forEach((p,i)=>{
    const b=document.createElement('button');
    b.className='preset-btn';
    b.innerHTML='<div class="preset-dot" style="background:rgb('+p.r+','+p.g+','+p.b+')"></div><span class="preset-label">'+p.label+'<br>'+p.K+'</span>';
    b.onclick=()=>{
      curR=p.r;curG=p.g;curB=p.b;
      setPreview(curR,curG,curB);
      document.querySelectorAll('.preset-btn').forEach(x=>x.classList.remove('active'));
      b.classList.add('active');
      sendColor(curR,curG,curB);
    };
    c.appendChild(b);
  });
}
function drawWheel(){
  const cv=document.getElementById('wheel');
  const ctx=cv.getContext('2d');
  const cx=110,cy=110,radius=106;
  for(let a=0;a<360;a++){
    const s=a*Math.PI/180,e=(a+2)*Math.PI/180;
    for(let ri=0;ri<radius;ri++){
      const sat=ri/radius;
      const grad=ctx.createRadialGradient(cx,cy,ri,cx,cy,ri+1);
      const col='hsl('+a+','+Math.round(sat*100)+'%,50%)';
      grad.addColorStop(0,col);grad.addColorStop(1,col);
      ctx.beginPath();ctx.arc(cx,cy,ri+1,s,e);ctx.arc(cx,cy,ri,e,s,true);
      ctx.fillStyle=grad;ctx.fill();
    }
  }
  const white=ctx.createRadialGradient(cx,cy,0,cx,cy,radius);
  white.addColorStop(0,'rgba(255,255,255,0.95)');
  white.addColorStop(0.25,'rgba(255,255,255,0.4)');
  white.addColorStop(1,'rgba(255,255,255,0)');
  ctx.beginPath();ctx.arc(cx,cy,radius,0,Math.PI*2);
  ctx.fillStyle=white;ctx.fill();
}
function pickColor(e){
  const cv=document.getElementById('wheel');
  const rect=cv.getBoundingClientRect();
  const touch=e.touches?e.touches[0]:e;
  const x=touch.clientX-rect.left,y=touch.clientY-rect.top;
  const scale=cv.width/rect.width;
  const px=Math.round(x*scale),py=Math.round(y*scale);
  const d=cv.getContext('2d').getImageData(px,py,1,1).data;
  if(d[3]<10)return;
  curR=d[0];curG=d[1];curB=d[2];
  setPreview(curR,curG,curB);
  document.querySelectorAll('.preset-btn').forEach(x=>x.classList.remove('active'));
  sendColor(curR,curG,curB);
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

            client.println(); break;
          }
          else currentLine = "";
        }
        else if (c != '\r') currentLine += c;

        // on/off
        if (currentLine.endsWith("/on")) {
          fill_solid(leds, NUM_LEDS, CRGB::White);
          FastLED.show();
        }
        if (currentLine.endsWith("/off")) {
          fill_solid(leds, NUM_LEDS, CRGB::Black);
          FastLED.show();
        }

        // brightness
        if (currentLine.indexOf("/brightness?v=") >= 0 && currentLine.endsWith(" HTTP/1.1")) {
          int idx = currentLine.indexOf("?v=") + 3;
          int val = currentLine.substring(idx).toInt();
          FastLED.setBrightness(val);
          FastLED.show();
        }

        // color
        if (currentLine.indexOf("/color?r=") >= 0 && currentLine.endsWith(" HTTP/1.1")) {
          int ri = currentLine.indexOf("r=") + 2;
          int gi = currentLine.indexOf("g=") + 2;
          int bi = currentLine.indexOf("b=") + 2;
          int ampG = currentLine.indexOf("&g=");
          int ampB = currentLine.indexOf("&b=");
          int r = currentLine.substring(ri, ampG).toInt();
          int g = currentLine.substring(gi, ampB).toInt();
          int b = currentLine.substring(bi, currentLine.indexOf(" HTTP/1.1")).toInt();
          fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
          FastLED.show();
          Serial.print("Color: "); Serial.print(r); Serial.print(" "); Serial.print(g); Serial.print(" "); Serial.println(b);
        }
      }
    }

    client.stop();
    Serial.println("Client Disconnected.");
  }
}
