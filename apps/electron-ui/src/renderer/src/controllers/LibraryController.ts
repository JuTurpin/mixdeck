import type { Crate, CuePoint, Track } from '../../../preload'

// Controller (ADR-014) : logique métier pour la bibliothèque de sons
// (Story 5.1 — base SQLite, scan/import de dossiers ; Story 5.2 — BPM/clé
// lus des tags, crates ; Story 5.3 — cue points, par chemin de fichier).
export class LibraryController {
  getTracks(): Promise<Track[]> {
    return window.mixdeck.libraryGetTracks()
  }

  // Ouvre le sélecteur de dossier natif puis scanne le dossier choisi.
  // Retourne null si l'utilisateur annule, sinon la bibliothèque à jour —
  // même convention que DeckController.pickAndLoadTrack().
  pickAndScanFolder(): Promise<Track[] | null> {
    return window.mixdeck.libraryPickAndScanFolder()
  }

  getCrates(): Promise<Crate[]> {
    return window.mixdeck.libraryGetCrates()
  }

  createCrate(name: string): Promise<Crate[]> {
    return window.mixdeck.libraryCreateCrate(name)
  }

  assignTrackToCrate(trackId: number, crateId: number): Promise<Track[]> {
    return window.mixdeck.libraryAssignTrackToCrate(trackId, crateId)
  }

  removeTrackFromCrate(trackId: number, crateId: number): Promise<Track[]> {
    return window.mixdeck.libraryRemoveTrackFromCrate(trackId, crateId)
  }

  getCuePoints(filePath: string): Promise<CuePoint[]> {
    return window.mixdeck.libraryGetCuePoints(filePath)
  }

  setCuePoint(filePath: string, slotIndex: number, positionSeconds: number): Promise<CuePoint[]> {
    return window.mixdeck.librarySetCuePoint(filePath, slotIndex, positionSeconds)
  }

  clearCuePoint(filePath: string, slotIndex: number): Promise<CuePoint[]> {
    return window.mixdeck.libraryClearCuePoint(filePath, slotIndex)
  }
}
