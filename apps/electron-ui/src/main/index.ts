import { app, BrowserWindow, dialog, ipcMain } from 'electron'
import { join } from 'path'
import { startEngineEventPump } from './engineEvents'
import { getAllTracks, openLibraryDatabase, scanFolder, type Track } from './library'
import type Database from 'better-sqlite3'

function createWindow(): BrowserWindow {
  const mainWindow = new BrowserWindow({
    width: 900,
    height: 600,
    show: false,
    backgroundColor: '#0d0e11',
    // La fenêtre native n'a pas de titre à afficher — TitleBar.tsx fournit déjà
    // la barre décorative (feux macOS factices) prévue par le mock (§6).
    frame: false,
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      sandbox: false
    }
  })

  mainWindow.once('ready-to-show', () => mainWindow.show())

  const rendererUrl = process.env['ELECTRON_RENDERER_URL']
  if (!app.isPackaged && rendererUrl) {
    mainWindow.loadURL(rendererUrl)
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }

  // Sans cadre natif, TitleBar.tsx (feux macOS factices) relaie ces 3
  // commandes — usage strictement personnel mono-fenêtre (ADR-010) : fermer
  // quitte toute l'app, pas seulement la fenêtre (voir window-all-closed).
  ipcMain.on('mixdeck:windowMinimize', () => mainWindow.minimize())
  ipcMain.on('mixdeck:windowToggleMaximize', () => {
    if (mainWindow.isMaximized()) mainWindow.unmaximize()
    else mainWindow.maximize()
  })
  ipcMain.on('mixdeck:windowClose', () => mainWindow.close())

  return mainWindow
}

// Story 2.4 — relai IPC vers le Bridge natif (Story 2.2). Le module ne peut
// être require()-é en sécurité que dans ce process (contextIsolation activée
// côté fenêtre) ; chaque handler est une traduction directe, une ligne,
// aucune logique métier (ADR-014) — celle-ci vit dans les Controllers du
// renderer (src/renderer/src/controllers/).
function createNativeEngine(): any {
  const bridgePath = join(
    __dirname,
    '../../../../native/engine/build-electron/Release/mixdeck_bridge.node'
  )
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const { NativeEngine } = require(bridgePath)
  const nativeEngine = new NativeEngine()

  // Story 4.4.1 — chemin de l'exécutable worker utilisé pour l'isolation des
  // plugins VST3 (voir PluginHost::setWorkerExecutablePath). Calculé comme
  // bridgePath ci-dessus : le process Node/Electron lui-même ne renseigne
  // rien d'utile sur l'arborescence de build CMake (voir aussi
  // setHostWindowHandle plus bas, même raisonnement pour un handle natif).
  const pluginWorkerPath = join(
    __dirname,
    '../../../../native/engine/build-electron/MixDeckPluginWorker_artefacts/Release/MixDeck Plugin Worker.app/Contents/MacOS/MixDeck Plugin Worker'
  )
  nativeEngine.setPluginWorkerExecutablePath(pluginWorkerPath)

  return nativeEngine
}

function registerBridgeHandlers(nativeEngine: any): void {
  const deckMethods = [
    'deckLoadTrack',
    'deckUnloadTrack',
    'deckPlay',
    'deckPause',
    'deckStop',
    'deckSeek',
    'deckGetState',
    'deckGetPosition',
    'deckGetLength',
    'deckGetBpm',
    'deckSetFilter',
    'deckSetPitch',
    'deckSetPitchMode',
    // Story 4.3 — chaîne d'effets par deck (deckIndex est toujours le 1er arg).
    'deckAddPlugin',
    'deckIsAddingPlugin',
    'deckGetLastPluginAddError',
    'deckRemovePlugin',
    'deckMovePlugin',
    'deckSetPluginBypassed',
    'deckShowPluginEditor',
    'deckHidePluginEditor',
    'deckGetPluginChain'
  ] as const

  const mixerMethods = [
    'mixerSetDeckVolume',
    'mixerSetCrossfaderPosition',
    'mixerSetCrossfaderCurve'
  ] as const

  // Story 4.3 — chaîne d'effets du bus master (même forme que deckXxx, sans index).
  const masterMethods = [
    'masterAddPlugin',
    'masterIsAddingPlugin',
    'masterGetLastPluginAddError',
    'masterRemovePlugin',
    'masterMovePlugin',
    'masterSetPluginBypassed',
    'masterShowPluginEditor',
    'masterHidePluginEditor',
    'masterGetPluginChain'
  ] as const

  // Story 4.1 — découverte de plugins (scan des dossiers standards VST3/AU).
  const pluginScanMethods = [
    'startPluginScan',
    'isPluginScanInProgress',
    'getAvailablePlugins',
    'addPluginFromPath' // Story 4.2 (ADR-004) — sélection manuelle, VST3 uniquement
  ] as const

  for (const method of [...deckMethods, ...mixerMethods, ...masterMethods, ...pluginScanMethods]) {
    ipcMain.handle(`mixdeck:${method}`, (_event, ...args) => nativeEngine[method](...args))
  }
}

// Story 2.5 — le mock Claude Design ne montre pas de champ de saisie pour
// charger un morceau (ça viendrait de la bibliothèque, Epic 5, absente ici) :
// un sélecteur de fichier natif le remplace. Capacité OS/Electron, pas
// logique métier.
function registerFilePickerHandler(): void {
  ipcMain.handle('mixdeck:pickFile', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openFile'],
      filters: [{ name: 'Audio', extensions: ['wav', 'mp3', 'flac', 'aif', 'aiff'] }]
    })
    return result.canceled ? null : result.filePaths[0]
  })
}

// Story 4.2 (ADR-004) — sélection manuelle du chemin d'un plugin. VST3
// uniquement (voir PluginHost::addPluginFromPath) — un .vst3 est un bundle
// macOS, sélectionnable comme un fichier unique malgré 'openFile'.
function registerPluginFilePickerHandler(): void {
  ipcMain.handle('mixdeck:pickPluginFile', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openFile'],
      filters: [{ name: 'VST3', extensions: ['vst3'] }]
    })
    return result.canceled ? null : result.filePaths[0]
  })
}

// Story 5.1 (ADR-007) — bibliothèque de sons, base SQLite locale. Indépendant
// du Bridge natif (pas de traitement de signal, voir library.ts) : son propre
// try/catch, une panne de l'un ne doit pas empêcher l'autre de fonctionner.
function registerLibraryHandlers(db: Database.Database): void {
  ipcMain.handle('mixdeck:libraryGetTracks', (): Track[] => getAllTracks(db))
  ipcMain.handle(
    'mixdeck:libraryPickAndScanFolder',
    async (): Promise<Track[] | null> => {
      const result = await dialog.showOpenDialog({ properties: ['openDirectory'] })
      if (result.canceled) return null
      return scanFolder(db, result.filePaths[0])
    }
  )
}

app.whenReady().then(() => {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  let nativeEngine: any
  try {
    nativeEngine = createNativeEngine()
    registerBridgeHandlers(nativeEngine)
    registerFilePickerHandler()
    registerPluginFilePickerHandler()
    console.log('[MixDeck] Bridge OK, deck A state =', nativeEngine.deckGetState(0))
  } catch (error) {
    console.error('[MixDeck] Bridge failed to load:', error)
  }

  try {
    registerLibraryHandlers(openLibraryDatabase())
  } catch (error) {
    console.error('[MixDeck] Bibliothèque (SQLite) indisponible:', error)
  }

  const mainWindow = createWindow()

  // Story 4.0 — donne au Bridge le handle natif (NSView* sur macOS) de la
  // fenêtre Electron, pour que les éditeurs de plugin réels (Story 4.3)
  // puissent s'y incruster (voir NodeBinding.cpp::SetHostWindowHandle et
  // ::DeckShowPluginEditor/::MasterShowPluginEditor). Direct, pas besoin
  // d'IPC : le process principal a déjà accès aux deux objets ici.
  if (nativeEngine) {
    nativeEngine.setHostWindowHandle(mainWindow.getNativeWindowHandle())
  }

  // Story 2.6 (ADR-015) — pousse les changements d'état/position au renderer
  // au lieu qu'il les demande (voir src/main/engineEvents.ts).
  if (nativeEngine) {
    const stopEventPump = startEngineEventPump(nativeEngine, mainWindow.webContents)
    mainWindow.on('closed', stopEventPump)
  }

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Usage strictement personnel, mono-fenêtre (ADR-010) : fermer la fenêtre
// quitte toute l'application, y compris sur macOS (pas le comportement
// standard qui laisserait l'app active en arrière-plan).
app.on('window-all-closed', () => {
  app.quit()
})
