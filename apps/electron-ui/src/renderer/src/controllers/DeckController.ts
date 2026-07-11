// Controller (ADR-014) : logique métier entre React et le Bridge (relai IPC
// exposé par src/preload/index.ts). Un DeckController par deck (A = 0, B = 1).
export class DeckController {
  constructor(private readonly deckIndex: number) {}

  loadTrack(path: string): Promise<string> {
    return window.mixdeck.deckLoadTrack(this.deckIndex, path)
  }

  // Ouvre le sélecteur de fichier natif puis charge le morceau choisi.
  // Retourne null si l'utilisateur annule, sinon le message d'erreur du
  // Bridge ("" = succès) — même convention que loadTrack().
  async pickAndLoadTrack(): Promise<string | null> {
    const path = await window.mixdeck.pickFile()
    if (!path) return null
    return this.loadTrack(path)
  }

  unloadTrack(): Promise<void> {
    return window.mixdeck.deckUnloadTrack(this.deckIndex)
  }

  play(): Promise<void> {
    return window.mixdeck.deckPlay(this.deckIndex)
  }

  pause(): Promise<void> {
    return window.mixdeck.deckPause(this.deckIndex)
  }

  stop(): Promise<void> {
    return window.mixdeck.deckStop(this.deckIndex)
  }

  seek(seconds: number): Promise<void> {
    return window.mixdeck.deckSeek(this.deckIndex, seconds)
  }

  setFilter(value: number): Promise<void> {
    return window.mixdeck.deckSetFilter(this.deckIndex, value)
  }

  setPitch(percent: number): Promise<void> {
    return window.mixdeck.deckSetPitch(this.deckIndex, percent)
  }

  getState(): Promise<string> {
    return window.mixdeck.deckGetState(this.deckIndex)
  }

  getPosition(): Promise<number> {
    return window.mixdeck.deckGetPosition(this.deckIndex)
  }

  getLength(): Promise<number> {
    return window.mixdeck.deckGetLength(this.deckIndex)
  }
}
