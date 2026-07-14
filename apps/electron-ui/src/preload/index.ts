import { contextBridge, ipcRenderer, webUtils } from 'electron'
import { MIXDECK_EVENT_CHANNEL, type MixdeckEvent } from '../shared/events'

// Story 4.1 — sous-ensemble de juce::PluginDescription utile côté JS.
export interface AvailablePlugin {
  name: string
  manufacturerName: string
  pluginFormatName: string
  fileOrIdentifier: string
}

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
  // Story 3.2 — 0 tant que l'analyse BPM en tâche de fond n'est pas terminée.
  deckGetBpm: (deckIndex: number): Promise<number> =>
    ipcRenderer.invoke('mixdeck:deckGetBpm', deckIndex),
  deckSetFilter: (deckIndex: number, value: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetFilter', deckIndex, value),
  deckSetPitch: (deckIndex: number, percent: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetPitch', deckIndex, percent),
  // Story 3.1 (ADR-006) — bascule entre pitch "vitesse liée" (défaut) et
  // "indépendant" (SoundTouch, tempo sans décaler la hauteur).
  deckSetPitchMode: (deckIndex: number, mode: 'linked' | 'independent'): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetPitchMode', deckIndex, mode),
  mixerSetDeckVolume: (deckIndex: number, volume: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetDeckVolume', deckIndex, volume),
  mixerSetCrossfaderPosition: (position: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetCrossfaderPosition', position),
  mixerSetCrossfaderCurve: (curve: string): Promise<void> =>
    ipcRenderer.invoke('mixdeck:mixerSetCrossfaderCurve', curve),
  // Story 4.0 — spike : incrustation d'une vue de test JUCE dans la fenêtre
  // Electron (voir NodeBinding.cpp::PluginWindowSpike). À retirer/remplacer
  // par la vraie chaîne d'effets en 4.3.
  showPluginWindowSpike: (): Promise<void> => ipcRenderer.invoke('mixdeck:showPluginWindowSpike'),
  hidePluginWindowSpike: (): Promise<void> => ipcRenderer.invoke('mixdeck:hidePluginWindowSpike'),
  // Story 4.1 — découverte des plugins VST3/AU (dossiers standards, ADR-003).
  // Pas de chargement dans une chaîne d'effets encore (Story 4.3).
  startPluginScan: (): Promise<void> => ipcRenderer.invoke('mixdeck:startPluginScan'),
  isPluginScanInProgress: (): Promise<boolean> => ipcRenderer.invoke('mixdeck:isPluginScanInProgress'),
  getAvailablePlugins: (): Promise<AvailablePlugin[]> =>
    ipcRenderer.invoke('mixdeck:getAvailablePlugins'),
  // Story 4.2 (ADR-004) — sélection manuelle du chemin d'un plugin, VST3
  // uniquement (voir PluginHost::addPluginFromPath). Retourne le message
  // d'erreur du Bridge ("" = succès), même convention que deckLoadTrack.
  pickPluginFile: (): Promise<string | null> => ipcRenderer.invoke('mixdeck:pickPluginFile'),
  addPluginFromPath: (path: string): Promise<string> =>
    ipcRenderer.invoke('mixdeck:addPluginFromPath', path),
  // Glisser-déposer : sous contextIsolation, un File du DOM n'expose plus son
  // chemin réel directement — webUtils.getPathForFile() est le remplacement
  // sûr exposé aux scripts preload.
  getPathForFile: (file: File): string => webUtils.getPathForFile(file),
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
