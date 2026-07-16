import { contextBridge, ipcRenderer, webUtils } from 'electron'
import { MIXDECK_EVENT_CHANNEL, type MixdeckEvent } from '../shared/events'
import type { Crate, Track } from '../main/library'

export type { Crate, Track }

// Story 4.1 — sous-ensemble de juce::PluginDescription utile côté JS.
// identifierString (4.3) est l'identifiant stable à repasser à addPlugin.
export interface AvailablePlugin {
  name: string
  manufacturerName: string
  pluginFormatName: string
  fileOrIdentifier: string
  identifierString: string
}

// Story 4.3 — un slot de la chaîne d'effets (deck ou bus master).
// isolated/crashed ajoutés en Story 4.4.1 (isolation hors-process VST3).
export interface PluginChainSlot {
  name: string
  bypassed: boolean
  isolated: boolean
  crashed: boolean
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
  // Story 4.1 — découverte des plugins VST3/AU (dossiers standards, ADR-003).
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
  // Story 4.3 — chaîne d'effets par deck : instanciation réelle (asynchrone,
  // voir isAddingPlugin/getLastPluginAddError), mutations synchrones
  // (remove/move/bypass), et ouverture de l'éditeur réel du plugin, incrusté
  // dans cette fenêtre (voir setHostWindowHandle côté main).
  deckAddPlugin: (deckIndex: number, identifier: string): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckAddPlugin', deckIndex, identifier),
  deckIsAddingPlugin: (deckIndex: number): Promise<boolean> =>
    ipcRenderer.invoke('mixdeck:deckIsAddingPlugin', deckIndex),
  deckGetLastPluginAddError: (deckIndex: number): Promise<string> =>
    ipcRenderer.invoke('mixdeck:deckGetLastPluginAddError', deckIndex),
  deckRemovePlugin: (deckIndex: number, index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckRemovePlugin', deckIndex, index),
  deckMovePlugin: (deckIndex: number, fromIndex: number, toIndex: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckMovePlugin', deckIndex, fromIndex, toIndex),
  deckSetPluginBypassed: (deckIndex: number, index: number, bypassed: boolean): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckSetPluginBypassed', deckIndex, index, bypassed),
  deckShowPluginEditor: (deckIndex: number, index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckShowPluginEditor', deckIndex, index),
  deckHidePluginEditor: (deckIndex: number, index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:deckHidePluginEditor', deckIndex, index),
  deckGetPluginChain: (deckIndex: number): Promise<PluginChainSlot[]> =>
    ipcRenderer.invoke('mixdeck:deckGetPluginChain', deckIndex),
  // Story 4.3 — chaîne d'effets du bus master (même forme, sans index de deck).
  masterAddPlugin: (identifier: string): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterAddPlugin', identifier),
  masterIsAddingPlugin: (): Promise<boolean> => ipcRenderer.invoke('mixdeck:masterIsAddingPlugin'),
  masterGetLastPluginAddError: (): Promise<string> =>
    ipcRenderer.invoke('mixdeck:masterGetLastPluginAddError'),
  masterRemovePlugin: (index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterRemovePlugin', index),
  masterMovePlugin: (fromIndex: number, toIndex: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterMovePlugin', fromIndex, toIndex),
  masterSetPluginBypassed: (index: number, bypassed: boolean): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterSetPluginBypassed', index, bypassed),
  masterShowPluginEditor: (index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterShowPluginEditor', index),
  masterHidePluginEditor: (index: number): Promise<void> =>
    ipcRenderer.invoke('mixdeck:masterHidePluginEditor', index),
  masterGetPluginChain: (): Promise<PluginChainSlot[]> => ipcRenderer.invoke('mixdeck:masterGetPluginChain'),
  // Glisser-déposer : sous contextIsolation, un File du DOM n'expose plus son
  // chemin réel directement — webUtils.getPathForFile() est le remplacement
  // sûr exposé aux scripts preload.
  getPathForFile: (file: File): string => webUtils.getPathForFile(file),
  // Story 2.5 — sélecteur de fichier natif (dialog.showOpenDialog côté main).
  pickFile: (): Promise<string | null> => ipcRenderer.invoke('mixdeck:pickFile'),
  // Story 5.1 (ADR-007) — bibliothèque de sons (SQLite local, voir
  // src/main/library.ts). libraryPickAndScanFolder retourne null si
  // l'utilisateur annule le sélecteur de dossier, même convention que
  // pickFile/pickPluginFile.
  libraryGetTracks: (): Promise<Track[]> => ipcRenderer.invoke('mixdeck:libraryGetTracks'),
  libraryPickAndScanFolder: (): Promise<Track[] | null> =>
    ipcRenderer.invoke('mixdeck:libraryPickAndScanFolder'),
  // Story 5.2 — crates (tags multiples par piste, recherche/filtre côté client).
  libraryGetCrates: (): Promise<Crate[]> => ipcRenderer.invoke('mixdeck:libraryGetCrates'),
  libraryCreateCrate: (name: string): Promise<Crate[]> => ipcRenderer.invoke('mixdeck:libraryCreateCrate', name),
  libraryAssignTrackToCrate: (trackId: number, crateId: number): Promise<Track[]> =>
    ipcRenderer.invoke('mixdeck:libraryAssignTrackToCrate', trackId, crateId),
  libraryRemoveTrackFromCrate: (trackId: number, crateId: number): Promise<Track[]> =>
    ipcRenderer.invoke('mixdeck:libraryRemoveTrackFromCrate', trackId, crateId),
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
