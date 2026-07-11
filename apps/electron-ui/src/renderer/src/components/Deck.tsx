// Composant Deck partagé A/B, paramétré par accent/label (architecture.md
// §6). Pas de BPM/clé, waveform, hot cues, EQ ni chaîne d'effets : ces
// éléments n'ont aucun moteur derrière aujourd'hui (voir docs/decision.md,
// discussion Story 2.5) et arriveront avec leurs stories respectives.
import { useMemo, useRef, useState, type PointerEvent } from 'react'
import { DeckController } from '../controllers/DeckController'
import type { MixerController } from '../controllers/MixerController'
import { useDeckState } from '../state'
import FilterKnob from './FilterKnob'

interface DeckProps {
  label: string
  accent: string
  deckIndex: number
  mixer: MixerController
}

function formatSeconds(seconds: number): string {
  const safe = Number.isFinite(seconds) && seconds > 0 ? seconds : 0
  const total = Math.floor(safe)
  return `${String(Math.floor(total / 60)).padStart(2, '0')}:${String(total % 60).padStart(2, '0')}`
}

export default function Deck({ label, accent, deckIndex, mixer }: DeckProps) {
  const controller = useMemo(() => new DeckController(deckIndex), [deckIndex])
  const { state, position, length } = useDeckState(controller)
  const [error, setError] = useState('')
  const [filterValue, setFilterValue] = useState(0)
  const [pitchValue, setPitchValue] = useState(0)
  const dragStart = useRef<{ x: number; position: number } | null>(null)

  const hasTrack = state !== 'EMPTY'

  const handleLoadClick = async (): Promise<void> => {
    const result = await controller.pickAndLoadTrack()
    if (result !== null) setError(result)
  }

  const handlePlatterDown = (event: PointerEvent<HTMLDivElement>): void => {
    if (!hasTrack) return
    dragStart.current = { x: event.clientX, position }
    event.currentTarget.setPointerCapture(event.pointerId)
  }

  const handlePlatterMove = (event: PointerEvent<HTMLDivElement>): void => {
    if (!dragStart.current) return
    const deltaSeconds = (event.clientX - dragStart.current.x) / 4
    const target = Math.min(length, Math.max(0, dragStart.current.position + deltaSeconds))
    controller.seek(target)
  }

  const handlePlatterUp = (event: PointerEvent<HTMLDivElement>): void => {
    dragStart.current = null
    event.currentTarget.releasePointerCapture(event.pointerId)
  }

  return (
    <div
      style={{
        flex: 1,
        background: '#14161b',
        borderRadius: 8,
        padding: 16,
        display: 'flex',
        flexDirection: 'column',
        gap: 10
      }}
    >
      <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
        <span style={{ width: 8, height: 8, borderRadius: '50%', background: accent }} />
        <span style={{ fontSize: 12, fontWeight: 600, letterSpacing: '0.08em', color: '#8b8f98' }}>
          {label}
        </span>
      </div>

      <div
        onClick={handleLoadClick}
        style={{
          fontSize: 13,
          color: hasTrack ? '#e8e9ec' : '#8b8f98',
          cursor: 'pointer',
          textAlign: 'center',
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          whiteSpace: 'nowrap'
        }}
        title="Cliquer pour charger un fichier audio"
      >
        {hasTrack ? state : 'Aucune piste chargée'}
      </div>
      {error && <span style={{ color: accent, fontSize: 11, textAlign: 'center' }}>{error}</span>}

      <div
        onPointerDown={handlePlatterDown}
        onPointerMove={handlePlatterMove}
        onPointerUp={handlePlatterUp}
        style={{
          width: 104,
          height: 104,
          borderRadius: '50%',
          border: `4px solid ${accent}`,
          background: '#101012',
          position: 'relative',
          margin: '4px auto',
          cursor: hasTrack ? 'grab' : 'default',
          touchAction: 'none',
          animation: 'mixdeck-platter-spin 2.5s linear infinite',
          animationPlayState: state === 'PLAYING' ? 'running' : 'paused'
        }}
      >
        <div
          style={{
            position: 'absolute',
            top: '50%',
            left: '50%',
            width: 2,
            height: 46,
            background: '#c9cdd4',
            transformOrigin: 'top center',
            transform: 'translate(-50%, 0)'
          }}
        />
      </div>

      <div style={{ textAlign: 'center', fontFamily: 'ui-monospace, monospace', fontSize: 12, color: '#8b8f98' }}>
        {formatSeconds(position)} / {formatSeconds(length)}
      </div>

      <div style={{ display: 'flex', gap: 8, justifyContent: 'center' }}>
        <button onClick={() => controller.play()} disabled={!hasTrack}>
          Play
        </button>
        <button onClick={() => controller.pause()} disabled={!hasTrack}>
          Pause
        </button>
      </div>
      <div style={{ display: 'flex', gap: 8, justifyContent: 'center', opacity: 0.7 }}>
        <button onClick={() => controller.stop()} disabled={!hasTrack} style={{ fontSize: 11 }}>
          Stop
        </button>
        <button
          onClick={() => controller.unloadTrack()}
          disabled={!hasTrack}
          style={{ fontSize: 11 }}
        >
          Unload
        </button>
      </div>

      <label style={{ fontSize: 10, color: '#8b8f98', textAlign: 'center', display: 'block' }}>
        Pitch
        <input
          type="range"
          min={-50}
          max={50}
          step={1}
          value={pitchValue}
          onChange={(e) => {
            const v = Number(e.target.value)
            setPitchValue(v)
            controller.setPitch(v)
          }}
          style={{ width: '100%' }}
        />
      </label>

      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 16 }}>
        <label style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4 }}>
          <input
            type="range"
            min={0}
            max={1}
            step={0.01}
            defaultValue={1}
            onChange={(e) => mixer.setDeckVolume(deckIndex, Number(e.target.value))}
            style={{ writingMode: 'vertical-lr', direction: 'rtl', width: 20, height: 104 }}
          />
          <span style={{ fontSize: 10, color: '#8b8f98' }}>VOL</span>
        </label>

        <FilterKnob
          value={filterValue}
          onChange={(v) => {
            setFilterValue(v)
            controller.setFilter(v)
          }}
          label="FILTRE"
        />
      </div>
    </div>
  )
}
