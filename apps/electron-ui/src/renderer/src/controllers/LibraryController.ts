import type { Track } from '../../../preload'

// Controller (ADR-014) : logique métier pour la bibliothèque de sons
// (Story 5.1 — base SQLite, scan/import de dossiers ; tags/crates/cue points
// arrivent en Story 5.2).
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
}
