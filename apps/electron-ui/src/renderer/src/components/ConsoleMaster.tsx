// Colonne centrale "console master" (architecture.md §6, ~170px). Sans
// vumètres ni knobs Master/Booth/Cue Mix : pas de mesure de niveau ni de
// bus master/booth dans le moteur aujourd'hui.
import { useMemo, useState } from 'react'
import Crossfader from './Crossfader'
import type { MixerController } from '../controllers/MixerController'
import { PluginController } from '../controllers/PluginController'
import type { AvailablePlugin } from '../../../preload'

interface ConsoleMasterProps {
  mixer: MixerController
}

export default function ConsoleMaster({ mixer }: ConsoleMasterProps) {
  // Story 4.0 — bouton temporaire pour valider le spike d'incrustation JUCE
  // dans la fenêtre Electron. À retirer/remplacer par la vraie chaîne
  // d'effets en 4.3.
  const [pluginSpikeVisible, setPluginSpikeVisible] = useState(false)

  const togglePluginSpike = (): void => {
    if (pluginSpikeVisible) window.mixdeck.hidePluginWindowSpike()
    else window.mixdeck.showPluginWindowSpike()
    setPluginSpikeVisible((v) => !v)
  }

  // Story 4.1 — bouton temporaire pour valider la découverte de plugins
  // (scan VST3/AU). À retirer/remplacer par un vrai navigateur de plugins
  // en 4.2/4.3.
  const pluginController = useMemo(() => new PluginController(), [])
  const [scanning, setScanning] = useState(false)
  const [foundPlugins, setFoundPlugins] = useState<AvailablePlugin[]>([])

  const handleScanClick = async (): Promise<void> => {
    setScanning(true)
    await pluginController.scan()

    const poll = setInterval(async () => {
      if (await pluginController.isScanInProgress()) return
      clearInterval(poll)
      setFoundPlugins(await pluginController.getAvailablePlugins())
      setScanning(false)
    }, 500)
  }

  return (
    <div
      style={{
        width: 170,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        background: '#16181d',
        borderRadius: 8,
        padding: 16,
        gap: 16
      }}
    >
      <Crossfader mixer={mixer} />
      <button onClick={togglePluginSpike} style={{ fontSize: 11 }}>
        {pluginSpikeVisible ? 'Masquer spike (4.0)' : 'Tester spike plugin (4.0)'}
      </button>
      <button onClick={handleScanClick} disabled={scanning} style={{ fontSize: 11 }}>
        {scanning ? 'Scan en cours...' : 'Scanner les plugins (4.1)'}
      </button>
      {foundPlugins.length > 0 && (
        <div
          style={{
            fontSize: 10,
            color: '#8b8f98',
            maxHeight: 120,
            overflowY: 'auto',
            width: '100%'
          }}
        >
          {foundPlugins.map((plugin) => (
            <div key={plugin.fileOrIdentifier}>
              {plugin.pluginFormatName} - {plugin.name}
            </div>
          ))}
        </div>
      )}
    </div>
  )
}
