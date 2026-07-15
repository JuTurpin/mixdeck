// Colonne centrale "console master" (architecture.md §6, ~170px). Sans
// vumètres ni knobs Master/Booth/Cue Mix : pas de mesure de niveau ni de
// bus master/booth dans le moteur aujourd'hui.
import { useMemo, useState, type DragEvent } from 'react'
import Crossfader from './Crossfader'
import PluginChainPanel from './PluginChainPanel'
import type { MixerController } from '../controllers/MixerController'
import { PluginController } from '../controllers/PluginController'
import { MasterBusController } from '../controllers/MasterBusController'
import type { AvailablePlugin } from '../../../preload'

interface ConsoleMasterProps {
  mixer: MixerController
  // Story 4.3 — remonte la liste découverte (scan/glisser-déposer) au parent
  // commun (App.tsx), pour que les PluginChainPanel des deux Deck puissent
  // aussi y piocher (même liste qu'ici, voir App.tsx).
  onAvailablePluginsChange: (plugins: AvailablePlugin[]) => void
}

export default function ConsoleMaster({ mixer, onAvailablePluginsChange }: ConsoleMasterProps) {
  // Story 4.1 — découverte des plugins (scan VST3/AU) + Story 4.2 (ADR-004,
  // sélection manuelle/glisser-déposer). Story 4.3 — la chaîne d'effets
  // elle-même (bus master) est déléguée à PluginChainPanel plus bas.
  const pluginController = useMemo(() => new PluginController(), [])
  const masterBusController = useMemo(() => new MasterBusController(), [])
  const [scanning, setScanning] = useState(false)
  const [foundPlugins, setFoundPlugins] = useState<AvailablePlugin[]>([])
  const [pluginError, setPluginError] = useState('')
  const [dragOver, setDragOver] = useState(false)

  const setFoundPluginsAndNotify = (plugins: AvailablePlugin[]): void => {
    setFoundPlugins(plugins)
    onAvailablePluginsChange(plugins)
  }

  const handleScanClick = async (): Promise<void> => {
    setScanning(true)
    await pluginController.scan()

    const poll = setInterval(async () => {
      if (await pluginController.isScanInProgress()) return
      clearInterval(poll)
      setFoundPluginsAndNotify(await pluginController.getAvailablePlugins())
      setScanning(false)
    }, 500)
  }

  // Story 4.2 (ADR-004) — sélection manuelle, VST3 uniquement (voir
  // PluginController.addPluginFromPath). Bouton et glisser-déposer partagent
  // ce même point d'entrée une fois le chemin en main.
  const handleAddResult = async (error: string | null): Promise<void> => {
    if (error === null) return // annulé par l'utilisateur (dialogue fermé)
    setPluginError(error)
    if (error === '') setFoundPluginsAndNotify(await pluginController.getAvailablePlugins())
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

      <PluginChainPanel
        controller={masterBusController}
        availablePlugins={foundPlugins}
        title="CHAÎNE D'EFFETS (MASTER)"
      />
    </div>
  )
}
