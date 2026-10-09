// Browser check of the web build's sound (headless Chromium over CDP).
//
//   python3 port/tools/serve_web.py --port 8791 &
//   node port/tools/web_audio_check.mjs <chrome or chrome-headless-shell> \
//        http://localhost:8791/opeu.html?autostart [seconds]
//
// Before the page loads, AudioContext is wrapped so that every buffer the
// page's ScriptProcessorNode (SDL3's Emscripten audio backend) produces is
// measured. Prints, every 6 s, the buffers delivered, the RMS and peak of
// the output (full scale 1.0) and the context state; exits 1 if the page
// produced no audible audio. Autoplay is allowed by a browser flag; one
// click answers the game's input prompt (mouse), after which the New Game
// screen starts the first song.
import { spawn } from 'node:child_process';
import { mkdtempSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const [chrome, url, secondsArg] = process.argv.slice(2);
if (!chrome || !url) {
  console.error('usage: node web_audio_check.mjs <chrome> <url> [seconds]');
  process.exit(2);
}
const seconds = Number(secondsArg || 24);
const port = 9333;
const profile = mkdtempSync(join(tmpdir(), 'opeu-chrome-'));
const browser = spawn(chrome, [`--remote-debugging-port=${port}`, '--autoplay-policy=no-user-gesture-required',
  '--no-first-run', `--user-data-dir=${profile}`, 'about:blank'], { stdio: 'ignore' });
const sleep = ms => new Promise(r => setTimeout(r, ms));

let target;
for (let i = 0; i < 50 && !target; i++) {
  await sleep(200);
  try {
    target = (await (await fetch(`http://127.0.0.1:${port}/json/list`)).json()).find(t => t.type === 'page');
  } catch (e) { /* not up yet */ }
}
if (!target) { console.error('no browser page'); browser.kill(); process.exit(2); }
const ws = new WebSocket(target.webSocketDebuggerUrl);
await new Promise(r => { ws.onopen = r; });
let next = 0;
const pending = new Map();
ws.onmessage = m => {
  const d = JSON.parse(m.data);
  if (d.id && pending.has(d.id)) { pending.get(d.id)(d); pending.delete(d.id); }
};
const send = (method, params = {}) => new Promise(r => {
  const id = ++next; pending.set(id, r); ws.send(JSON.stringify({ id, method, params }));
});

const measure = `
window.__opeuAudio = {buffers: 0, frames: 0, sumsq: 0, peak: 0, audible: 0, rate: 0, ctx: null};
const OriginalAudioContext = window.AudioContext;
window.AudioContext = window.webkitAudioContext = class extends OriginalAudioContext {
  constructor(...a) { super(...a); window.__opeuAudio.rate = this.sampleRate; window.__opeuAudio.ctx = this; }
  createScriptProcessor(...a) {
    const node = super.createScriptProcessor(...a);
    Object.defineProperty(node, 'onaudioprocess', { set(fn) {
      node.addEventListener('audioprocess', e => {
        fn.call(node, e);
        const b = e.outputBuffer.getChannelData(0), A = window.__opeuAudio;
        let s = 0;
        for (let i = 0; i < b.length; i++) { s += b[i] * b[i]; A.peak = Math.max(A.peak, Math.abs(b[i])); }
        A.buffers++; A.frames += b.length; A.sumsq += s;
        if (Math.sqrt(s / b.length) > 0.003) A.audible++;
      });
    }, get() { return null; } });
    return node;
  }
};`;
await send('Page.enable');
await send('Page.addScriptToEvaluateOnNewDocument', { source: measure });
await send('Page.navigate', { url });
await sleep(3000);
for (const type of ['mousePressed', 'mouseReleased'])
  await send('Input.dispatchMouseEvent', { type, x: 300, y: 300, button: 'left', clickCount: 1 });
let last;
for (let t = 0; t < seconds; t += 6) {
  await sleep(6000);
  const r = await send('Runtime.evaluate', { expression: `(() => { const A = window.__opeuAudio; return JSON.stringify({
    buffers: A.buffers, frames: A.frames, audible: A.audible, rms: +Math.sqrt(A.sumsq / Math.max(1, A.frames)).toFixed(4),
    peak: +A.peak.toFixed(4), rate: A.rate, state: A.ctx && A.ctx.state }); })()` });
  last = JSON.parse(r.result.result.value);
  console.log(`${t + 6} s:`, JSON.stringify(last));
}
ws.close();
browser.kill();
const ok = last && last.state === 'running' && last.audible > 0 && last.rms > 0.003;
console.log(ok ? 'web audio check passed' : 'WEB AUDIO CHECK FAILED');
process.exit(ok ? 0 : 1);
