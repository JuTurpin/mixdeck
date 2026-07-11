// Story 2.4 — UI de test minimale pour valider React -> Controller -> IPC ->
// Bridge -> moteur JUCE de bout en bout. Pas la vraie UI (composants bruts,
// pas de style Claude Design) : ça arrive en Story 2.5, en réutilisant les
// mêmes Controllers/state ci-dessous.
import { useMemo, useState } from 'react'
import { DeckController, MixerController } from './controllers'
import { useDeckState } from './state'

function formatSeconds(seconds: number): string {
  const safeSeconds = Number.isFinite(seconds) && seconds > 0 ? seconds : 0
  const total = Math.floor(safeSeconds)
  const minutes = Math.floor(total / 60)
  const secs = total % 60
  return `${String(minutes).padStart(2, '0')}:${String(secs).padStart(2, '0')}`
}

interface DeckTestPanelProps {
  label: string
  deckIndex: number
  mixer: MixerController
}

function DeckTestPanel({ label, deckIndex, mixer }: DeckTestPanelProps): JSX.Element {
  const controller = useMemo(() => new DeckController(deckIndex), [deckIndex])
  const { state, position, length } = useDeckState(controller)
  const [path, setPath] = useState('')
  const [error, setError] = useState('')

  const handleLoad = async (): Promise<void> => {
    setError(await controller.loadTrack(path))
  }

  return (
    <div
      style={{
        flex: 1,
        padding: 16,
        background: '#14161b',
        borderRadius: 8,
        display: 'flex',
        flexDirection: 'column',
        gap: 8
      }}
    >
      <h2 style={{ margin: 0, fontSize: 14, letterSpacing: '0.08em', color: '#8b8f98' }}>{label}</h2>

      <input
        value={path}
        onChange={(e) => setPath(e.target.value)}
        placeholder="Chemin du fichier audio"
        style={{
          padding: 6,
          background: '#0d0e11',
          color: '#e8e9ec',
          border: '1px solid #2a2d35',
          borderRadius: 4
        }}
      />
      <button onClick={handleLoad}>Load</button>
      {error && <span style={{ color: '#f0955a', fontSize: 12 }}>{error}</span>}

      <div style={{ fontFamily: 'ui-monospace, monospace', fontSize: 12 }}>
        {state} — {formatSeconds(position)} / {formatSeconds(length)}
      </div>

      <div style={{ display: 'flex', gap: 8 }}>
        <button onClick={() => controller.play()}>Play</button>
        <button onClick={() => controller.pause()}>Pause</button>
        <button onClick={() => controller.stop()}>Stop</button>
        <button onClick={() => controller.unloadTrack()}>Unload</button>
      </div>

      <label style={{ fontSize: 12, color: '#8b8f98' }}>
        Volume
        <input
          type="range"
          min={0}
          max={1}
          step={0.01}
          defaultValue={1}
          onChange={(e) => mixer.setDeckVolume(deckIndex, Number(e.target.value))}
        />
      </label>
      <label style={{ fontSize: 12, color: '#8b8f98' }}>
        Filtre
        <input
          type="range"
          min={-1}
          max={1}
          step={0.01}
          defaultValue={0}
          onChange={(e) => controller.setFilter(Number(e.target.value))}
        />
      </label>
      <label style={{ fontSize: 12, color: '#8b8f98' }}>
        Pitch
        <input
          type="range"
          min={-50}
          max={50}
          step={1}
          defaultValue={0}
          onChange={(e) => controller.setPitch(Number(e.target.value))}
        />
      </label>
    </div>
  )
}

export default function App(): JSX.Element {
  const mixer = useMemo(() => new MixerController(), [])

  return (
    <div
      style={{
        minHeight: '100vh',
        background: '#0d0e11',
        color: '#e8e9ec',
        fontFamily: 'system-ui, -apple-system, sans-serif',
        padding: 16,
        boxSizing: 'border-box'
      }}
    >
      <h1 style={{ textAlign: 'center', fontWeight: 600, letterSpacing: '0.02em', fontSize: 18 }}>
        MixDeck — harnais de test Story 2.4
      </h1>
      <div style={{ display: 'flex', gap: 16 }}>
        <DeckTestPanel label="DECK A" deckIndex={0} mixer={mixer} />

        <div
          style={{
            width: 160,
            display: 'flex',
            flexDirection: 'column',
            gap: 8,
            alignItems: 'center'
          }}
        >
          <label style={{ fontSize: 12, color: '#8b8f98' }}>Crossfader</label>
          <input
            type="range"
            min={0}
            max={1}
            step={0.01}
            defaultValue={0.5}
            onChange={(e) => mixer.setCrossfaderPosition(Number(e.target.value))}
          />
          <select
            defaultValue="smooth"
            onChange={(e) => mixer.setCrossfaderCurve(e.target.value)}
          >
            <option value="linear">Linéaire</option>
            <option value="smooth">Smooth</option>
            <option value="cut">Cut</option>
          </select>
        </div>

        <DeckTestPanel label="DECK B" deckIndex={1} mixer={mixer} />
      </div>
    </div>
  )
}
