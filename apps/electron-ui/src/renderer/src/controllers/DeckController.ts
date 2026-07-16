import type { PluginChainSlot } from '../../../preload'

// Controller (ADR-014) : logique métier entre React et le Bridge (relai IPC
// exposé par src/preload/index.ts). Un DeckController par deck (A = 0, B = 1).
export class DeckController {
  // public : useDeckState.ts (Story 2.6) filtre les événements poussés par
  // deckIndex.
  constructor(readonly deckIndex: number) {}

  loadTrack(path: string): Promise<string> {
    return window.mixdeck.deckLoadTrack(this.deckIndex, path)
  }

  // Ouvre le sélecteur de fichier natif puis charge le morceau choisi.
  // Retourne null si l'utilisateur annule, sinon le chemin choisi et le
  // message d'erreur du Bridge ("" = succès) — le chemin est nécessaire à
  // l'appelant pour remonter "quel fichier est chargé sur ce deck" (Story
  // 5.3, cue points) plutôt que de le redemander séparément.
  async pickAndLoadTrack(): Promise<{ path: string; error: string } | null> {
    const path = await window.mixdeck.pickFile()
    if (!path) return null
    const error = await this.loadTrack(path)
    return { path, error }
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

  // Story 3.1 (ADR-006) — bascule entre pitch "vitesse liée" (défaut) et
  // "indépendant" (SoundTouch). Pas encore d'appelant UI (backend only).
  setPitchMode(mode: 'linked' | 'independent'): Promise<void> {
    return window.mixdeck.deckSetPitchMode(this.deckIndex, mode)
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

  // Story 3.2 — 0 tant que l'analyse BPM en tâche de fond n'est pas terminée.
  getBpm(): Promise<number> {
    return window.mixdeck.deckGetBpm(this.deckIndex)
  }

  // Story 3.3 — pourcentage à appliquer (via setPitch) pour que ownBpm (BPM
  // brut détecté) rejoigne otherEffectiveBpm (BPM effectif actuel de l'autre
  // deck), clampé à la plage du slider de pitch existant (-50..+50).
  computeSyncPitch(ownBpm: number, otherEffectiveBpm: number): number {
    const raw = (otherEffectiveBpm / ownBpm - 1) * 100
    return Math.max(-50, Math.min(50, raw))
  }

  // Story 4.3 — chaîne d'effets de ce deck. addPlugin est asynchrone
  // (instanciation potentiellement lente, voir isAddingPlugin/getLastAddError,
  // même convention que PluginController.scan()) ; le reste est synchrone.
  addPlugin(identifier: string): Promise<void> {
    return window.mixdeck.deckAddPlugin(this.deckIndex, identifier)
  }

  isAddingPlugin(): Promise<boolean> {
    return window.mixdeck.deckIsAddingPlugin(this.deckIndex)
  }

  getLastPluginAddError(): Promise<string> {
    return window.mixdeck.deckGetLastPluginAddError(this.deckIndex)
  }

  removePlugin(index: number): Promise<void> {
    return window.mixdeck.deckRemovePlugin(this.deckIndex, index)
  }

  movePlugin(fromIndex: number, toIndex: number): Promise<void> {
    return window.mixdeck.deckMovePlugin(this.deckIndex, fromIndex, toIndex)
  }

  setPluginBypassed(index: number, bypassed: boolean): Promise<void> {
    return window.mixdeck.deckSetPluginBypassed(this.deckIndex, index, bypassed)
  }

  showPluginEditor(index: number): Promise<void> {
    return window.mixdeck.deckShowPluginEditor(this.deckIndex, index)
  }

  hidePluginEditor(index: number): Promise<void> {
    return window.mixdeck.deckHidePluginEditor(this.deckIndex, index)
  }

  getPluginChain(): Promise<PluginChainSlot[]> {
    return window.mixdeck.deckGetPluginChain(this.deckIndex)
  }
}
