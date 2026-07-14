// Colonne centrale "console master" (architecture.md §6, ~170px). Sans
// vumètres ni knobs Master/Booth/Cue Mix : pas de mesure de niveau ni de
// bus master/booth dans le moteur aujourd'hui.
import { useState } from 'react'
import Crossfader from './Crossfader'
import type { MixerController } from '../controllers/MixerController'

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
    </div>
  )
}
