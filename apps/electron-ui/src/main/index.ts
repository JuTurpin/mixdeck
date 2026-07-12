import { app, BrowserWindow, dialog, ipcMain } from 'electron'
import { join } from 'path'
import { startEngineEventPump } from './engineEvents'

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
  return new NativeEngine()
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
    'deckSetFilter',
    'deckSetPitch',
    'deckSetPitchMode'
  ] as const

  const mixerMethods = [
    'mixerSetDeckVolume',
    'mixerSetCrossfaderPosition',
    'mixerSetCrossfaderCurve'
  ] as const

  for (const method of [...deckMethods, ...mixerMethods]) {
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

app.whenReady().then(() => {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  let nativeEngine: any
  try {
    nativeEngine = createNativeEngine()
    registerBridgeHandlers(nativeEngine)
    registerFilePickerHandler()
    console.log('[MixDeck] Bridge OK, deck A state =', nativeEngine.deckGetState(0))
  } catch (error) {
    console.error('[MixDeck] Bridge failed to load:', error)
  }

  const mainWindow = createWindow()

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
