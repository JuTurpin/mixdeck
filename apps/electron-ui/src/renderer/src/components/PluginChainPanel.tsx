// Story 4.3 — chaîne d'effets partagée par Deck (par deck) et ConsoleMaster
// (bus master) : même forme dans les deux cas (DeckController expose ses
// méthodes deckXxxPlugin* sous ces noms, MasterBusController les expose sans
// index — les deux satisfont PluginChainController par typage structurel).
// Visuel calqué sur la section "CHAÎNE D'EFFETS" du mock Claude Design
// (design/export-html/deck_component.html, §fxSlots) : slot sombre, nom
// cliquable (ouvre l'éditeur réel du plugin, incrusté dans cette fenêtre),
// bouton "B" (bypass), "×" (retirer). Réordonnancement via flèches
// haut/bas — pas de glisser-déposer (scope validé pour cette story).
import { useEffect, useState } from 'react'
import type { AvailablePlugin, PluginChainSlot } from '../../../preload'

export interface PluginChainController {
  addPlugin(identifier: string): Promise<void>
  isAddingPlugin(): Promise<boolean>
  getLastPluginAddError(): Promise<string>
  removePlugin(index: number): Promise<void>
  movePlugin(fromIndex: number, toIndex: number): Promise<void>
  setPluginBypassed(index: number, bypassed: boolean): Promise<void>
  showPluginEditor(index: number): Promise<void>
  hidePluginEditor(index: number): Promise<void>
  getPluginChain(): Promise<PluginChainSlot[]>
}

interface PluginChainPanelProps {
  controller: PluginChainController
  availablePlugins: AvailablePlugin[]
  title?: string
}

export default function PluginChainPanel({
  controller,
  availablePlugins,
  title = "CHAÎNE D'EFFETS"
}: PluginChainPanelProps) {
  const [slots, setSlots] = useState<PluginChainSlot[]>([])
  const [selectedIdentifier, setSelectedIdentifier] = useState('')
  const [adding, setAdding] = useState(false)
  const [error, setError] = useState('')
  // Le slot dont l'éditeur est actuellement affiché (un seul à la fois) — le
  // réduire (voir handleMinimizeEditor) le remet à null sans détruire
  // l'éditeur natif ni son réglage (voir PluginChain::hidePluginEditor).
  const [activeEditorIndex, setActiveEditorIndex] = useState<number | null>(null)

  const refreshSlots = async (): Promise<void> => {
    setSlots(await controller.getPluginChain())
  }

  useEffect(() => {
    // Le controller identifie le deck (ou le bus master) : un changement
    // d'identité recharge la chaîne correspondante.
    refreshSlots()
  }, [controller])

  useEffect(() => {
    if (!selectedIdentifier && availablePlugins.length > 0) {
      setSelectedIdentifier(availablePlugins[0].identifierString)
    }
  }, [availablePlugins, selectedIdentifier])

  const handleAdd = async (): Promise<void> => {
    if (!selectedIdentifier || adding) return
    setAdding(true)
    setError('')
    await controller.addPlugin(selectedIdentifier)

    const poll = setInterval(async () => {
      if (await controller.isAddingPlugin()) return
      clearInterval(poll)
      setError(await controller.getLastPluginAddError())
      await refreshSlots()
      setAdding(false)
    }, 300)
  }

  const handleRemove = async (index: number): Promise<void> => {
    if (activeEditorIndex === index) setActiveEditorIndex(null)
    await controller.removePlugin(index)
    await refreshSlots()
  }

  const handleMove = async (index: number, delta: -1 | 1): Promise<void> => {
    const target = index + delta
    if (target < 0 || target >= slots.length) return
    // Les indices bougent : si l'éditeur du slot déplacé est affiché, le
    // réduire proprement d'abord — sinon sa fenêtre resterait visible à
    // l'écran alors que l'UI aurait perdu sa trace (plus moyen de la réduire).
    if (activeEditorIndex === index) {
      await controller.hidePluginEditor(index)
      setActiveEditorIndex(null)
    }
    await controller.movePlugin(index, target)
    await refreshSlots()
  }

  const handleToggleBypass = async (index: number, slot: PluginChainSlot): Promise<void> => {
    await controller.setPluginBypassed(index, !slot.bypassed)
    await refreshSlots()
  }

  // Ramène l'éditeur de ce plugin au premier plan (le crée s'il n'existe pas
  // encore, le réutilise sinon — voir PluginChain::showPluginEditor).
  // Toujours appelé, même si l'état local pense déjà ce slot actif : l'éditeur
  // a aussi son propre bouton "réduire" natif (dans son en-tête, voir
  // PluginChain::EditorHost) qui peut le masquer sans que React ne le sache —
  // showPluginEditor est idempotent côté natif (setVisible(true) + toFront),
  // donc revenir dessus ici est sûr et garantit que ça rétablit toujours.
  const handleShowEditor = async (index: number): Promise<void> => {
    if (activeEditorIndex !== null && activeEditorIndex !== index)
      await controller.hidePluginEditor(activeEditorIndex)
    await controller.showPluginEditor(index)
    setActiveEditorIndex(index)
  }

  // Masque l'éditeur sans le détruire ni retirer le plugin de la chaîne — le
  // rétablir ensuite (handleShowEditor) réutilise le même éditeur, mêmes réglages.
  const handleMinimizeEditor = async (index: number): Promise<void> => {
    await controller.hidePluginEditor(index)
    setActiveEditorIndex(null)
  }

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 6, minWidth: 0 }}>
      <span style={{ font: '500 9px system-ui', color: '#565a63', letterSpacing: '0.06em' }}>
        {title}
      </span>

      {slots.map((slot, index) => (
        <div
          key={index}
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: 6,
            background: '#0d0e11',
            borderRadius: 6,
            padding: '6px 8px',
            minWidth: 0
          }}
        >
          <div
            onClick={() => handleShowEditor(index)}
            title={
              slot.crashed
                ? 'Le process de ce plugin a planté — retirez-le et rajoutez-le pour réessayer'
                : activeEditorIndex === index
                  ? 'Éditeur déjà au premier plan'
                  : "Ouvrir l'éditeur du plugin"
            }
            style={{
              flex: 1,
              cursor: 'pointer',
              font: '500 11px system-ui',
              color: slot.crashed
                ? '#f0955a'
                : slot.bypassed
                  ? '#565a63'
                  : activeEditorIndex === index
                    ? '#4fd1c5'
                    : '#c9cdd4',
              whiteSpace: 'nowrap',
              overflow: 'hidden',
              textOverflow: 'ellipsis',
              minWidth: 0
            }}
          >
            {slot.isolated && (
              <span
                title="Plugin isolé dans son propre process (VST3, Story 4.4.1)"
                style={{ fontSize: 8, color: '#565a63', marginRight: 4 }}
              >
                ISO
              </span>
            )}
            {slot.name}
            {slot.crashed && ' (planté)'}
          </div>
          {activeEditorIndex === index && (
            <button
              onClick={() => handleMinimizeEditor(index)}
              title="Réduire l'éditeur (le plugin reste actif)"
              style={{ width: 18, height: 18, flex: 'none', borderRadius: 4, border: 'none', background: 'transparent', color: '#4fd1c5', fontSize: 11, cursor: 'pointer', lineHeight: 1, padding: 0 }}
            >
              ─
            </button>
          )}
          <button
            onClick={() => handleMove(index, -1)}
            disabled={index === 0}
            title="Monter"
            style={{ width: 18, height: 18, flex: 'none', borderRadius: 4, border: 'none', background: 'transparent', color: '#565a63', fontSize: 9, cursor: 'pointer', lineHeight: 1, padding: 0 }}
          >
            ▲
          </button>
          <button
            onClick={() => handleMove(index, 1)}
            disabled={index === slots.length - 1}
            title="Descendre"
            style={{ width: 18, height: 18, flex: 'none', borderRadius: 4, border: 'none', background: 'transparent', color: '#565a63', fontSize: 9, cursor: 'pointer', lineHeight: 1, padding: 0 }}
          >
            ▼
          </button>
          <button
            onClick={() => handleToggleBypass(index, slot)}
            title="Bypass"
            style={{
              width: 20,
              height: 18,
              flex: 'none',
              borderRadius: 4,
              border: 'none',
              background: slot.bypassed ? '#565a63' : '#4fd1c5',
              color: '#0d0e11',
              font: '700 8px system-ui',
              cursor: 'pointer'
            }}
          >
            B
          </button>
          <button
            onClick={() => handleRemove(index)}
            title="Retirer"
            style={{ width: 18, height: 18, flex: 'none', borderRadius: 4, border: 'none', background: 'transparent', color: '#565a63', font: '700 12px system-ui', cursor: 'pointer', lineHeight: 1 }}
          >
            ×
          </button>
        </div>
      ))}

      <div style={{ display: 'flex', gap: 6, alignItems: 'center', minWidth: 0 }}>
        <select
          value={selectedIdentifier}
          onChange={(e) => setSelectedIdentifier(e.target.value)}
          disabled={availablePlugins.length === 0}
          style={{ flex: 1, fontSize: 10, minWidth: 0 }}
        >
          {availablePlugins.length === 0 && <option value="">Aucun plugin trouvé</option>}
          {availablePlugins.map((plugin) => (
            <option key={plugin.identifierString} value={plugin.identifierString}>
              {plugin.pluginFormatName} - {plugin.name}
            </option>
          ))}
        </select>
        <button onClick={handleAdd} disabled={adding || !selectedIdentifier} style={{ fontSize: 10 }}>
          {adding ? '...' : '+ Ajouter'}
        </button>
      </div>
      {error && <span style={{ color: '#f0955a', fontSize: 10 }}>{error}</span>}
    </div>
  )
}
