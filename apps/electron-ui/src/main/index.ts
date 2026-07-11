import { app, BrowserWindow } from 'electron'
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

// Story 2.3 — simple diagnostic de compatibilité ABI : le Bridge (Story 2.2)
// doit être reconstruit contre le Node d'Electron (voir native/engine,
// `cmake-js compile --runtime=electron`, sortie dans build-electron/) pour se
// charger ici. N'expose rien au renderer et ne bloque jamais l'ouverture de la
// fenêtre si ça échoue — le vrai câblage arrive en Story 2.4/2.5.
function checkBridge(): void {
  try {
    const bridgePath = join(
      __dirname,
      '../../../../native/engine/build-electron/Release/mixdeck_bridge.node'
    )
    // eslint-disable-next-line @typescript-eslint/no-var-requires
    const { NativeEngine } = require(bridgePath)
    const engine = new NativeEngine()
    console.log('[MixDeck] Bridge OK, deck A state =', engine.deckGetState(0))
  } catch (error) {
    console.error('[MixDeck] Bridge failed to load:', error)
  }
}

app.whenReady().then(() => {
  checkBridge()
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
