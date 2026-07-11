// Manual validation script for Story 2.2 (Bridge N-API) — no Electron/UI yet,
// just enough to confirm the addon relays commands to the existing engine.
// Usage: node test-bridge.js /path/to/a.wav /path/to/b.wav

const { NativeEngine } = require('./build/Release/mixdeck_bridge.node');

const [pathA, pathB] = process.argv.slice(2);
if (!pathA || !pathB) {
  console.error('Usage: node test-bridge.js /path/to/a.wav /path/to/b.wav');
  process.exit(1);
}

const engine = new NativeEngine();

console.log('Deck A load error (empty = ok):', JSON.stringify(engine.deckLoadTrack(0, pathA)));
console.log('Deck B load error (empty = ok):', JSON.stringify(engine.deckLoadTrack(1, pathB)));

engine.mixerSetDeckVolume(0, 1.0);
engine.mixerSetDeckVolume(1, 1.0);
engine.mixerSetCrossfaderPosition(0.5);
engine.mixerSetCrossfaderCurve('smooth');

engine.deckPlay(0);
engine.deckPlay(1);

setInterval(() => {
  const a = `A[${engine.deckGetState(0)}] ${engine.deckGetPosition(0).toFixed(1)}/${engine.deckGetLength(0).toFixed(1)}`;
  const b = `B[${engine.deckGetState(1)}] ${engine.deckGetPosition(1).toFixed(1)}/${engine.deckGetLength(1).toFixed(1)}`;
  console.log(a, '   ', b);
}, 1000);
