// Story 5.1 — bibliothèque de sons, périmètre volontairement minimal (base
// SQLite + scan/import de dossiers, voir le plan de la story) : pas de
// sidebar crates, pas de recherche, pas de colonnes BPM/Clé — tout ça est la
// Story 5.2 (tags/crates/cue points). Juste de quoi scanner un dossier et
// charger un morceau trouvé sur un deck, bout en bout.
import { useEffect, useMemo, useState } from 'react'
import type { Track } from '../../../preload'
import { DeckController } from '../controllers/DeckController'
import { LibraryController } from '../controllers/LibraryController'

function formatDuration(seconds: number | null): string {
  if (seconds === null || !Number.isFinite(seconds)) return '--:--'
  const total = Math.floor(seconds)
  return `${String(Math.floor(total / 60)).padStart(2, '0')}:${String(total % 60).padStart(2, '0')}`
}

export default function LibraryPanel() {
  const controller = useMemo(() => new LibraryController(), [])
  // Wrappers légers sans état propre (DeckController ne fait que relayer
  // window.mixdeck.deckXxx(deckIndex, ...)) — pas besoin de les partager avec
  // Deck.tsx, qui possède déjà les siens pour ses propres besoins.
  const deckA = useMemo(() => new DeckController(0), [])
  const deckB = useMemo(() => new DeckController(1), [])

  const [tracks, setTracks] = useState<Track[]>([])
  const [scanning, setScanning] = useState(false)
  const [error, setError] = useState('')

  useEffect(() => {
    controller.getTracks().then(setTracks)
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
  }

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
      <div style={{ maxHeight: 220, overflowY: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 12 }}>
          <thead>
            <tr style={{ color: '#565a63', textAlign: 'left' }}>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Titre</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Artiste</th>
              <th style={{ fontWeight: 500, padding: '4px 8px' }}>Durée</th>
              <th style={{ padding: '4px 8px' }} />
            </tr>
          </thead>
          <tbody>
            {tracks.length === 0 ? (
              <tr>
                <td colSpan={4} style={{ padding: '8px', color: '#565a63' }}>
                  Aucun morceau — scanner un dossier pour commencer.
                </td>
              </tr>
            ) : (
              tracks.map((track) => (
                <tr key={track.id} style={{ borderTop: '1px solid rgba(255,255,255,0.06)' }}>
                  <td
                    style={{
                      padding: '4px 8px',
                      color: '#c9cdd4',
                      whiteSpace: 'nowrap',
                      overflow: 'hidden',
                      textOverflow: 'ellipsis',
                      maxWidth: 220
                    }}
                  >
                    {track.title}
                  </td>
                  <td style={{ padding: '4px 8px', color: '#8b8f98' }}>{track.artist ?? ''}</td>
                  <td style={{ padding: '4px 8px', color: '#8b8f98', fontFamily: 'ui-monospace, monospace' }}>
                    {formatDuration(track.durationSeconds)}
                  </td>
                  <td style={{ padding: '4px 8px', display: 'flex', gap: 4 }}>
                    <button onClick={() => handleLoad(deckA, track.filePath)} style={{ fontSize: 10 }}>
                      → A
                    </button>
                    <button onClick={() => handleLoad(deckB, track.filePath)} style={{ fontSize: 10 }}>
                      → B
                    </button>
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  )
}
