// Story 2.5 — vraie UI, calquée sur l'export Claude Design (architecture.md
// §6), pour ce que le moteur pilote réellement aujourd'hui : Deck / Mixer /
// Filtre. BPM/clé, waveform, hot cues, EQ, effets, vumètres et bibliothèque
// n'ont pas encore de moteur derrière — absents de l'écran pour l'instant
// (voir le plan de cette story), pas de placeholders morts.
import { useMemo } from 'react'
import { TitleBar, Deck, ConsoleMaster } from './components'
import { MixerController } from './controllers'

export default function App() {
  const mixer = useMemo(() => new MixerController(), [])

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
        <Deck label="DECK A" accent="#4fd1c5" deckIndex={0} mixer={mixer} />
        <ConsoleMaster mixer={mixer} />
        <Deck label="DECK B" accent="#f0955a" deckIndex={1} mixer={mixer} />
      </div>
    </div>
  )
}
