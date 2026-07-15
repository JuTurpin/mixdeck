// Story 2.5 — vraie UI, calquée sur l'export Claude Design (architecture.md
// §6), pour ce que le moteur pilote réellement aujourd'hui : Deck / Mixer /
// Filtre. BPM/clé, waveform, hot cues, EQ, effets, vumètres et bibliothèque
// n'ont pas encore de moteur derrière — absents de l'écran pour l'instant
// (voir le plan de cette story), pas de placeholders morts.
import { useCallback, useMemo, useState } from 'react'
import { TitleBar, Deck, ConsoleMaster, type DeckSyncInfo } from './components'
import { MixerController } from './controllers'
import type { AvailablePlugin } from '../../preload'

const emptySyncInfo: DeckSyncInfo = { bpm: 0, pitch: 0 }

function sameSyncInfo(a: DeckSyncInfo, b: DeckSyncInfo): boolean {
  return a.bpm === b.bpm && a.pitch === b.pitch
}

export default function App() {
  const mixer = useMemo(() => new MixerController(), [])
  // Story 3.3 — seul état partagé entre les deux decks : ce qu'il faut pour
  // que le bouton Sync de l'un lise le BPM effectif de l'autre. Chaque Deck
  // remonte ici ce qu'il sait déjà (bpm/pitch), rien n'est recalculé ailleurs.
  const [syncInfo, setSyncInfo] = useState<[DeckSyncInfo, DeckSyncInfo]>([
    emptySyncInfo,
    emptySyncInfo
  ])
  // Story 4.3 — la découverte de plugins (scan/glisser-déposer) reste dans
  // ConsoleMaster (4.1/4.2) ; la liste trouvée est remontée ici pour que les
  // deux chaînes d'effets par deck puissent aussi y piocher (même liste,
  // partagée par tous les emplacements de la chaîne, deck ou bus master).
  const [availablePlugins, setAvailablePlugins] = useState<AvailablePlugin[]>([])

  // Références stables (jamais recréées) pour ne redéclencher l'effet de
  // remontée de Deck.tsx que lorsque bpm/pitch changent réellement.
  const handleSyncInfoChangeA = useCallback((info: DeckSyncInfo) => {
    setSyncInfo((prev) => (sameSyncInfo(prev[0], info) ? prev : [info, prev[1]]))
  }, [])
  const handleSyncInfoChangeB = useCallback((info: DeckSyncInfo) => {
    setSyncInfo((prev) => (sameSyncInfo(prev[1], info) ? prev : [prev[0], info]))
  }, [])

  return (
    <div
      style={{
        minHeight: '100vh',
        background: '#0d0e11',
        color: '#e8e9ec',
        fontFamily: 'system-ui, -apple-system, sans-serif',
        display: 'flex',
        flexDirection: 'column'
      }}
    >
      <TitleBar />
      <div style={{ display: 'flex', gap: 16, padding: 16, flex: 1 }}>
        <Deck
          label="DECK A"
          accent="#4fd1c5"
          deckIndex={0}
          mixer={mixer}
          otherDeck={syncInfo[1]}
          onSyncInfoChange={handleSyncInfoChangeA}
          availablePlugins={availablePlugins}
        />
        <ConsoleMaster mixer={mixer} onAvailablePluginsChange={setAvailablePlugins} />
        <Deck
          label="DECK B"
          accent="#f0955a"
          deckIndex={1}
          mixer={mixer}
          otherDeck={syncInfo[0]}
          onSyncInfoChange={handleSyncInfoChangeB}
          availablePlugins={availablePlugins}
        />
      </div>
    </div>
  )
}
