// Controller (ADR-014) : logique métier pour le mixer, entre React et le
// Bridge (relai IPC exposé par src/preload/index.ts).
export class MixerController {
  setDeckVolume(deckIndex: number, volume: number): Promise<void> {
    return window.mixdeck.mixerSetDeckVolume(deckIndex, volume)
  }

  setCrossfaderPosition(position: number): Promise<void> {
    return window.mixdeck.mixerSetCrossfaderPosition(position)
  }

  setCrossfaderCurve(curve: string): Promise<void> {
    return window.mixdeck.mixerSetCrossfaderCurve(curve)
  }
}
