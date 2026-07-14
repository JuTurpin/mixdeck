// Colonne centrale "console master" (architecture.md §6, ~170px). Sans
// vumètres ni knobs Master/Booth/Cue Mix : pas de mesure de niveau ni de
// bus master/booth dans le moteur aujourd'hui.
import { useMemo, useState, type DragEvent } from 'react'
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
  const [pluginError, setPluginError] = useState('')
  const [dragOver, setDragOver] = useState(false)

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

  // Story 4.2 (ADR-004) — sélection manuelle, VST3 uniquement (voir
  // PluginController.addPluginFromPath). Bouton et glisser-déposer partagent
  // ce même point d'entrée une fois le chemin en main.
  const handleAddResult = async (error: string | null): Promise<void> => {
    if (error === null) return // annulé par l'utilisateur (dialogue fermé)
    setPluginError(error)
    if (error === '') setFoundPlugins(await pluginController.getAvailablePlugins())
  }

  const handleBrowseClick = async (): Promise<void> => {
    await handleAddResult(await pluginController.pickAndAddPlugin())
  }

  const handleDragOver = (event: DragEvent<HTMLDivElement>): void => {
    event.preventDefault()
    setDragOver(true)
  }

  const handleDrop = async (event: DragEvent<HTMLDivElement>): Promise<void> => {
    event.preventDefault()
    setDragOver(false)
    for (const file of event.dataTransfer.files) {
      const path = window.mixdeck.getPathForFile(file)
      // eslint-disable-next-line no-await-in-loop
      await handleAddResult(await pluginController.addPluginFromPath(path))
    }
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
      <button onClick={handleBrowseClick} style={{ fontSize: 11 }}>
        Parcourir un plugin (4.2)
      </button>
      {pluginError && <span style={{ color: '#f0955a', fontSize: 10, textAlign: 'center' }}>{pluginError}</span>}
      <div
        onDragOver={handleDragOver}
        onDragLeave={() => setDragOver(false)}
        onDrop={handleDrop}
        title="Glisser un .vst3 ici"
        style={{
          fontSize: 10,
          color: '#8b8f98',
          maxHeight: 120,
          overflowY: 'auto',
          width: '100%',
          minHeight: 24,
          border: dragOver ? '1px dashed #4fd1c5' : '1px dashed transparent',
          borderRadius: 4,
          padding: 4
        }}
      >
        {foundPlugins.length === 0
          ? 'Glisser un .vst3 ici'
          : foundPlugins.map((plugin) => (
              <div key={plugin.fileOrIdentifier}>
                {plugin.pluginFormatName} - {plugin.name}
              </div>
            ))}
      </div>
    </div>
  )
}
