import { app, BrowserWindow, ipcMain } from 'electron'
import { join } from 'path'

function createWindow(): void {
  const mainWindow = new BrowserWindow({
    width: 900,
    height: 600,
    show: false,
    backgroundColor: '#0d0e11',
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
    'deckSetPitch'
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

app.whenReady().then(() => {
  try {
    const nativeEngine = createNativeEngine()
    registerBridgeHandlers(nativeEngine)
    console.log('[MixDeck] Bridge OK, deck A state =', nativeEngine.deckGetState(0))
  } catch (error) {
    console.error('[MixDeck] Bridge failed to load:', error)
  }

  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
