import { contextBridge, ipcRenderer } from 'electron'
import { MIXDECK_EVENT_CHANNEL, type MixdeckEvent } from '../shared/events'

// Story 2.4 — relai pur vers le process principal (voir src/main/index.ts) :
// chaque méthode ne fait que transmettre l'appel via IPC, aucune logique
// métier ici (ADR-014) — celle-ci vit dans les Controllers du renderer.
const mixdeck = {
  deckLoadTrack: (deckIndex: number, path: string): Promise<string> =>
    ipcRenderer.invoke('mixdeck:deckLoadTrack', deckIndex, path),
  deckUnloadTrack: (deckIndex: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckUnloadTrack', deckIndex),
  deckPlay: (deckIndex: number): Promise<void> => ipcRenderer.invoke('mixdeck:deckPlay', deckIndex),
  deckPause: (deckIndex: number): Promise<void> => ipcRenderer.invoke('mixdeck:deckPause', deckIndex),
  deckStop: (deckIndex: number): Promise<void> => ipcRenderer.invoke('mixdeck:deckStop', deckIndex),
  deckSeek: (deckIndex: number, seconds: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSeek', deckIndex, seconds),
  deckGetState: (deckIndex: number): Promise<string> =>
    ipcRenderer.invoke('mixdeck:deckGetState', deckIndex),
  deckGetPosition: (deckIndex: number): Promise<number> =>
    ipcRenderer.invoke('mixdeck:deckGetPosition', deckIndex),
  deckGetLength: (deckIndex: number): Promise<number> =>
    ipcRenderer.invoke('mixdeck:deckGetLength', deckIndex),
  deckSetFilter: (deckIndex: number, value: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetFilter', deckIndex, value),
  deckSetPitch: (deckIndex: number, percent: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetPitch', deckIndex, percent),
  mixerSetDeckVolume: (deckIndex: number, volume: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetDeckVolume', deckIndex, volume),
  mixerSetCrossfaderPosition: (position: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetCrossfaderPosition', position),
  mixerSetCrossfaderCurve: (curve: string): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetCrossfaderCurve', curve),
  // Story 2.5 — sélecteur de fichier natif (dialog.showOpenDialog côté main).
  pickFile: (): Promise<string | null> => ipcRenderer.invoke('mixdeck:pickFile'),
  // Story 2.5 — commandes de fenêtre (fenêtre sans cadre natif, TitleBar.tsx).
  windowMinimize: (): void => ipcRenderer.send('mixdeck:windowMinimize'),
  windowToggleMaximize: (): void => ipcRenderer.send('mixdeck:windowToggleMaximize'),
  windowClose: (): void => ipcRenderer.send('mixdeck:windowClose'),
  // Story 2.6 (ADR-015) — abonnement au flux d'événements poussé par le
  // process principal (src/main/engineEvents.ts). Retourne une fonction de
  // désabonnement ; on ne peut pas exposer ipcRenderer brut à travers
  // contextBridge, d'où ce wrapper.
  onEvent: (callback: (event: MixdeckEvent) => void): (() => void) => {
    const listener = (_event: Electron.IpcRendererEvent, payload: MixdeckEvent): void =>
      callback(payload)
    ipcRenderer.on(MIXDECK_EVENT_CHANNEL, listener)
    return () => ipcRenderer.removeListener(MIXDECK_EVENT_CHANNEL, listener)
  }
}

contextBridge.exposeInMainWorld('mixdeck', mixdeck)

export type MixdeckBridgeApi = typeof mixdeck
