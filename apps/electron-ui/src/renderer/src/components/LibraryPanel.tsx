// Story 5.1 — bibliothèque de sons (base SQLite + scan/import de dossiers).
// Story 5.2 — BPM/Clé (lus des tags, pas d'analyse audio), crates (tags
// multiples par piste, assignation/retrait/filtre) et recherche texte,
// tout côté client comme dans le mock Claude Design. Story 5.3 (cue points)
// signale à App.tsx quel chemin est chargé sur quel deck (onTrackLoadedToDeck)
// — les cue points eux-mêmes sont gérés/affichés dans Deck.tsx.
import { useEffect, useMemo, useState } from 'react'
import type { Crate, Track } from '../../../preload'
import { DeckController } from '../controllers/DeckController'
import { LibraryController } from '../controllers/LibraryController'

function formatDuration(seconds: number | null): string {
  if (seconds === null || !Number.isFinite(seconds)) return '--:--'
  const total = Math.floor(seconds)
  return `${String(Math.floor(total / 60)).padStart(2, '0')}:${String(total % 60).padStart(2, '0')}`
}

interface TrackRowProps {
  track: Track
  crates: Crate[]
  deckA: DeckController
  deckB: DeckController
  onLoad: (deck: DeckController, path: string) => void
  onAssign: (trackId: number, crateId: number) => void
  onRemoveCrate: (trackId: number, crateId: number) => void
}

function TrackRow({ track, crates, deckA, deckB, onLoad, onAssign, onRemoveCrate }: TrackRowProps) {
  const assignableCrates = crates.filter((c) => !track.crates.includes(c.name))
  const [selectedCrateId, setSelectedCrateId] = useState('')

  const handleAssignClick = (): void => {
    if (!selectedCrateId) return
    onAssign(track.id, Number(selectedCrateId))
    setSelectedCrateId('')
  }

  return (
    <tr style={{ borderTop: '1px solid rgba(255,255,255,0.06)' }}>
      <td
        style={{
          padding: '4px 8px',
          color: '#c9cdd4',
          whiteSpace: 'nowrap',
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          maxWidth: 200
        }}
      >
        {track.title}
      </td>
      <td style={{ padding: '4px 8px', color: '#8b8f98' }}>{track.artist ?? ''}</td>
      <td style={{ padding: '4px 8px', color: '#8b8f98', fontFamily: 'ui-monospace, monospace' }}>
        {formatDuration(track.durationSeconds)}
      </td>
      <td style={{ padding: '4px 8px', color: '#8b8f98', fontFamily: 'ui-monospace, monospace' }}>
        {track.bpm !== null ? track.bpm.toFixed(0) : '—'}
      </td>
      <td style={{ padding: '4px 8px', color: '#8b8f98', fontFamily: 'ui-monospace, monospace' }}>
        {track.key ?? '—'}
      </td>
      <td style={{ padding: '4px 8px' }}>
        <div style={{ display: 'flex', flexWrap: 'wrap', gap: 4, alignItems: 'center' }}>
          {track.crates.map((crateName) => {
            const crate = crates.find((c) => c.name === crateName)
            return (
              <span
                key={crateName}
                style={{
                  display: 'inline-flex',
                  alignItems: 'center',
                  gap: 3,
                  background: '#0d0e11',
                  borderRadius: 4,
                  padding: '1px 4px',
                  fontSize: 10,
                  color: '#c9cdd4'
                }}
              >
                {crateName}
                {crate && (
                  <button
                    onClick={() => onRemoveCrate(track.id, crate.id)}
                    title="Retirer de cette crate"
                    style={{
                      border: 'none',
                      background: 'transparent',
                      color: '#565a63',
                      cursor: 'pointer',
                      fontSize: 10,
                      padding: 0,
                      lineHeight: 1
                    }}
                  >
                    ×
                  </button>
                )}
              </span>
            )
          })}
          {assignableCrates.length > 0 && (
            <>
              <select
                value={selectedCrateId}
                onChange={(e) => setSelectedCrateId(e.target.value)}
                style={{ fontSize: 10 }}
              >
                <option value="">+ crate</option>
                {assignableCrates.map((c) => (
                  <option key={c.id} value={c.id}>
                    {c.name}
                  </option>
                ))}
              </select>
              <button onClick={handleAssignClick} disabled={!selectedCrateId} style={{ fontSize: 10 }}>
                Ajouter
              </button>
            </>
          )}
        </div>
      </td>
      <td style={{ padding: '4px 8px', display: 'flex', gap: 4 }}>
        <button onClick={() => onLoad(deckA, track.filePath)} style={{ fontSize: 10 }}>
          → A
        </button>
        <button onClick={() => onLoad(deckB, track.filePath)} style={{ fontSize: 10 }}>
          → B
        </button>
      </td>
    </tr>
  )
}

interface LibraryPanelProps {
  // Story 5.3 — signale à App.tsx quel chemin vient d'être chargé sur quel
  // deck, pour que Deck.tsx puisse retrouver les cue points de ce morceau
  // (voir App.tsx et Deck.tsx).
  onTrackLoadedToDeck: (deckIndex: number, path: string) => void
}

export default function LibraryPanel({ onTrackLoadedToDeck }: LibraryPanelProps) {
  const controller = useMemo(() => new LibraryController(), [])
  // Wrappers légers sans état propre (DeckController ne fait que relayer
  // window.mixdeck.deckXxx(deckIndex, ...)) — pas besoin de les partager avec
  // Deck.tsx, qui possède déjà les siens pour ses propres besoins.
  const deckA = useMemo(() => new DeckController(0), [])
  const deckB = useMemo(() => new DeckController(1), [])

  const [tracks, setTracks] = useState<Track[]>([])
  const [crates, setCrates] = useState<Crate[]>([])
  const [scanning, setScanning] = useState(false)
  const [error, setError] = useState('')
  const [search, setSearch] = useState('')
  const [crateFilter, setCrateFilter] = useState<number | null>(null)
  const [newCrateName, setNewCrateName] = useState('')

  useEffect(() => {
    controller.getTracks().then(setTracks)
    controller.getCrates().then(setCrates)
  }, [controller])

  const handleScan = async (): Promise<void> => {
    setScanning(true)
    setError('')
    const result = await controller.pickAndScanFolder()
    if (result !== null) setTracks(result)
    setScanning(false)
  }

  const handleLoad = async (deck: DeckController, path: string): Promise<void> => {
    const result = await deck.loadTrack(path)
    if (result) setError(result)
    else onTrackLoadedToDeck(deck.deckIndex, path)
  }

  const handleCreateCrate = async (): Promise<void> => {
    const name = newCrateName.trim()
    if (!name) return
    setCrates(await controller.createCrate(name))
    setNewCrateName('')
  }

  const handleAssign = async (trackId: number, crateId: number): Promise<void> => {
    setTracks(await controller.assignTrackToCrate(trackId, crateId))
  }

  const handleRemoveCrate = async (trackId: number, crateId: number): Promise<void> => {
    setTracks(await controller.removeTrackFromCrate(trackId, crateId))
  }

  // Recherche + filtre par crate : côté client, comme dans le mock
  // (buildLibraryRows) — pas de requête serveur par filtre.
  const activeCrateName = crateFilter !== null ? crates.find((c) => c.id === crateFilter)?.name : undefined
  const q = search.trim().toLowerCase()
  const filteredTracks = tracks.filter((track) => {
    if (activeCrateName && !track.crates.includes(activeCrateName)) return false
    if (q && !(track.title.toLowerCase().includes(q) || (track.artist ?? '').toLowerCase().includes(q)))
      return false
    return true
  })

  return (
    <div
      style={{
        background: '#14161b',
        borderRadius: 8,
        padding: 16,
        display: 'flex',
        flexDirection: 'column',
        gap: 10
      }}
    >
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <span style={{ fontSize: 12, fontWeight: 600, letterSpacing: '0.08em', color: '#8b8f98' }}>
          BIBLIOTHÈQUE
        </span>
        <button onClick={handleScan} disabled={scanning} style={{ fontSize: 11 }}>
          {scanning ? 'Scan en cours...' : 'Scanner un dossier...'}
        </button>
      </div>
      {error && <span style={{ color: '#f0955a', fontSize: 11 }}>{error}</span>}

      <div style={{ display: 'flex', alignItems: 'center', gap: 8, flexWrap: 'wrap' }}>
        <input
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          placeholder="Rechercher (titre, artiste)..."
          style={{ fontSize: 11, flex: '1 1 160px', minWidth: 0 }}
        />
        <button
          onClick={() => setCrateFilter(null)}
          style={{
            fontSize: 10,
            background: crateFilter === null ? '#2a2d34' : undefined,
            color: crateFilter === null ? '#eef0f2' : undefined
          }}
        >
          Toutes
        </button>
        {crates.map((crate) => (
          <button
            key={crate.id}
            onClick={() => setCrateFilter(crate.id)}
            style={{
              fontSize: 10,
              background: crateFilter === crate.id ? '#2a2d34' : undefined,
              color: crateFilter === crate.id ? '#eef0f2' : undefined
            }}
          >
            {crate.name}
          </button>
        ))}
        <input
          value={newCrateName}
          onChange={(e) => setNewCrateName(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === 'Enter') handleCreateCrate()
          }}
          placeholder="Nouvelle crate..."
          style={{ fontSize: 11, width: 120 }}
        />
        <button onClick={handleCreateCrate} disabled={!newCrateName.trim()} style={{ fontSize: 10 }}>
          Créer
        </button>
      </div>

      <div style={{ maxHeight: 260, overflowY: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 12 }}>
          <thead>
            <tr style={{ color: '#565a63', textAlign: 'left' }}>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Titre</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Artiste</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Durée</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>BPM</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Clé</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Crates</th>
              <th style={{ padding: '4px 8px' }} />
            </tr>
          </thead>
          <tbody>
            {filteredTracks.length === 0 ? (
              <tr>
                <td colSpan={7} style={{ padding: '8px', color: '#565a63' }}>
                  {tracks.length === 0 ? 'Aucun morceau — scanner un dossier pour commencer.' : 'Aucun résultat.'}
                </td>
              </tr>
            ) : (
              filteredTracks.map((track) => (
                <TrackRow
                  key={track.id}
                  track={track}
                  crates={crates}
                  deckA={deckA}
                  deckB={deckB}
                  onLoad={handleLoad}
                  onAssign={handleAssign}
                  onRemoveCrate={handleRemoveCrate}
                />
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  )
}
