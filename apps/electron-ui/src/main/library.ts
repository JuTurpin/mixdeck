import { app } from 'electron'
import Database from 'better-sqlite3'
import { basename, extname, join } from 'path'
import { readdir } from 'fs/promises'

// Story 5.1 (ADR-007) — bibliothèque de sons : base SQLite locale (better-sqlite3),
// entièrement côté Node/Electron (pas de traitement de signal, donc pas de
// raison de passer par le moteur natif, voir plan de la story). Portée
// volontairement minimale : chemin/titre/artiste/durée uniquement — tags,
// crates, cue points arrivent en Story 5.2.
export interface Track {
  id: number
  filePath: string
  title: string
  artist: string | null
  durationSeconds: number | null
  addedAt: string
}

// Mêmes extensions que le sélecteur de fichier existant (DeckController.pickAndLoadTrack).
const AUDIO_EXTENSIONS = new Set(['.wav', '.mp3', '.flac', '.aif', '.aiff'])

interface TrackRow {
  id: number
  file_path: string
  title: string
  artist: string | null
  duration_seconds: number | null
  added_at: string
}

function rowToTrack(row: TrackRow): Track {
  return {
    id: row.id,
    filePath: row.file_path,
    title: row.title,
    artist: row.artist,
    durationSeconds: row.duration_seconds,
    addedAt: row.added_at
  }
}

export function openLibraryDatabase(): Database.Database {
  const dbPath = join(app.getPath('userData'), 'library.sqlite3')
  const db = new Database(dbPath)
  db.exec(`
    CREATE TABLE IF NOT EXISTS tracks (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      file_path TEXT NOT NULL UNIQUE,
      title TEXT NOT NULL,
      artist TEXT,
      duration_seconds REAL,
      added_at TEXT NOT NULL DEFAULT (datetime('now'))
    )
  `)
  return db
}

export function getAllTracks(db: Database.Database): Track[] {
  const rows = db.prepare('SELECT * FROM tracks ORDER BY added_at DESC').all() as TrackRow[]
  return rows.map(rowToTrack)
}

async function findAudioFiles(folderPath: string): Promise<string[]> {
  const entries = await readdir(folderPath, { withFileTypes: true, recursive: true })
  const results: string[] = []
  for (const entry of entries) {
    if (!entry.isFile()) continue
    if (!AUDIO_EXTENSIONS.has(extname(entry.name).toLowerCase())) continue
    results.push(join(entry.parentPath, entry.name))
  }
  return results
}

// Marche récursive le dossier choisi, lit les tags de chaque fichier audio
// trouvé (music-metadata — import dynamique : le package est ESM-only, le
// process main est bundlé en CJS, `require()` échouerait) et upsert par
// chemin de fichier (rescanner le même dossier ne duplique pas). Un fichier
// individuel corrompu/sans tags ne doit jamais interrompre le scan des
// autres : repli sur le nom de fichier, artiste/durée laissés vides.
export async function scanFolder(db: Database.Database, folderPath: string): Promise<Track[]> {
  const { parseFile } = await import('music-metadata')
  const files = await findAudioFiles(folderPath)

  const upsert = db.prepare(`
    INSERT INTO tracks (file_path, title, artist, duration_seconds)
    VALUES (@filePath, @title, @artist, @durationSeconds)
    ON CONFLICT(file_path) DO UPDATE SET
      title = excluded.title,
      artist = excluded.artist,
      duration_seconds = excluded.duration_seconds
  `)

  for (const filePath of files) {
    let title = basename(filePath, extname(filePath))
    let artist: string | null = null
    let durationSeconds: number | null = null

    try {
      const metadata = await parseFile(filePath, { duration: true })
      if (metadata.common.title) title = metadata.common.title
      if (metadata.common.artist) artist = metadata.common.artist
      if (metadata.format.duration) durationSeconds = metadata.format.duration
    } catch {
      // Tags illisibles ou fichier corrompu — on garde le repli ci-dessus et
      // on continue avec les fichiers suivants.
    }

    upsert.run({ filePath, title, artist, durationSeconds })
  }

  return getAllTracks(db)
}
