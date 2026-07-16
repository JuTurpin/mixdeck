import { app } from 'electron'
import Database from 'better-sqlite3'
import { basename, extname, join } from 'path'
import { readdir } from 'fs/promises'

// Story 5.1 (ADR-007) — bibliothèque de sons : base SQLite locale (better-sqlite3),
// entièrement côté Node/Electron (pas de traitement de signal, donc pas de
// raison de passer par le moteur natif). Story 5.2 ajoute BPM/clé (lus des
// tags, pas d'analyse audio) et les crates (tags multiples par piste, voir
// le mock Claude Design). Cue points repoussés en Story 5.3 (touchent le
// Deck en exécution, nature différente).
export interface Track {
  id: number
  filePath: string
  title: string
  artist: string | null
  durationSeconds: number | null
  bpm: number | null
  key: string | null // notation Camelot, ex. "8A" — voir toCamelotKey()
  crates: string[]
  addedAt: string
}

export interface Crate {
  id: number
  name: string
}

// Mêmes extensions que le sélecteur de fichier existant (DeckController.pickAndLoadTrack).
const AUDIO_EXTENSIONS = new Set(['.wav', '.mp3', '.flac', '.aif', '.aiff'])

interface TrackRow {
  id: number
  file_path: string
  title: string
  artist: string | null
  duration_seconds: number | null
  bpm: number | null
  key_camelot: string | null
  crate_names: string | null // "Techno,Favoris" (GROUP_CONCAT) ou null si aucune
  added_at: string
}

function rowToTrack(row: TrackRow): Track {
  return {
    id: row.id,
    filePath: row.file_path,
    title: row.title,
    artist: row.artist,
    durationSeconds: row.duration_seconds,
    bpm: row.bpm,
    key: row.key_camelot,
    crates: row.crate_names ? row.crate_names.split(',') : [],
    addedAt: row.added_at
  }
}

function ensureColumn(db: Database.Database, table: string, column: string, definition: string): void {
  const columns = db.prepare(`PRAGMA table_info(${table})`).all() as { name: string }[]
  if (!columns.some((c) => c.name === column)) {
    db.exec(`ALTER TABLE ${table} ADD COLUMN ${column} ${definition}`)
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
    );
    CREATE TABLE IF NOT EXISTS crates (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL UNIQUE
    );
    CREATE TABLE IF NOT EXISTS track_crates (
      track_id INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,
      crate_id INTEGER NOT NULL REFERENCES crates(id) ON DELETE CASCADE,
      PRIMARY KEY (track_id, crate_id)
    );
  `)
  // Story 5.2 — colonnes ajoutées après coup sur une base qui peut déjà
  // exister depuis la Story 5.1 (pas de ALTER TABLE ADD COLUMN IF NOT EXISTS
  // en SQLite, d'où PRAGMA table_info).
  ensureColumn(db, 'tracks', 'bpm', 'REAL')
  ensureColumn(db, 'tracks', 'key_camelot', 'TEXT')
  return db
}

export function getAllTracks(db: Database.Database): Track[] {
  const rows = db
    .prepare(
      `SELECT t.*, GROUP_CONCAT(c.name) AS crate_names
       FROM tracks t
       LEFT JOIN track_crates tc ON tc.track_id = t.id
       LEFT JOIN crates c ON c.id = tc.crate_id
       GROUP BY t.id
       ORDER BY t.added_at DESC`
    )
    .all() as TrackRow[]
  return rows.map(rowToTrack)
}

export function getCrates(db: Database.Database): Crate[] {
  return db.prepare('SELECT * FROM crates ORDER BY name').all() as Crate[]
}

export function createCrate(db: Database.Database, name: string): Crate[] {
  const trimmed = name.trim()
  if (trimmed.length > 0) {
    db.prepare('INSERT OR IGNORE INTO crates (name) VALUES (?)').run(trimmed)
  }
  return getCrates(db)
}

export function assignTrackToCrate(db: Database.Database, trackId: number, crateId: number): Track[] {
  db.prepare('INSERT OR IGNORE INTO track_crates (track_id, crate_id) VALUES (?, ?)').run(trackId, crateId)
  return getAllTracks(db)
}

export function removeTrackFromCrate(db: Database.Database, trackId: number, crateId: number): Track[] {
  db.prepare('DELETE FROM track_crates WHERE track_id = ? AND crate_id = ?').run(trackId, crateId)
  return getAllTracks(db)
}

// Story 5.2 — conversion note musicale -> notation Camelot (roue des DJ),
// utilisée pour la clé lue depuis les tags (music-metadata: common.key, ex.
// "A Minor", "F#m", "Cmaj"). Table fixe à 24 entrées, aucune dépendance.
const PITCH_CLASS: Record<string, number> = {
  c: 0,
  'b#': 0,
  'c#': 1,
  db: 1,
  d: 2,
  'd#': 3,
  eb: 3,
  e: 4,
  fb: 4,
  f: 5,
  'e#': 5,
  'f#': 6,
  gb: 6,
  g: 7,
  'g#': 8,
  ab: 8,
  a: 9,
  'a#': 10,
  bb: 10,
  b: 11,
  cb: 11
}

const MAJOR_CAMELOT: Record<number, string> = {
  0: '8B',
  1: '3B',
  2: '10B',
  3: '5B',
  4: '12B',
  5: '7B',
  6: '2B',
  7: '9B',
  8: '4B',
  9: '11B',
  10: '6B',
  11: '1B'
}

const MINOR_CAMELOT: Record<number, string> = {
  0: '5A',
  1: '12A',
  2: '7A',
  3: '2A',
  4: '9A',
  5: '4A',
  6: '11A',
  7: '6A',
  8: '1A',
  9: '8A',
  10: '3A',
  11: '10A'
}

export function toCamelotKey(rawKey: string | undefined): string | null {
  if (!rawKey) return null
  const trimmed = rawKey.trim()

  // Déjà en notation Camelot (certains taggers l'écrivent directement).
  const camelotMatch = trimmed.match(/^(1[0-2]|[1-9])([ABab])$/)
  if (camelotMatch) return `${camelotMatch[1]}${camelotMatch[2].toUpperCase()}`

  const noteMatch = trimmed.match(/^([A-Ga-g])([#♯b♭]?)\s*(maj(?:or)?|min(?:or)?|m)?$/i)
  if (!noteMatch) return null

  const [, letter, accidentalRaw, modeRaw] = noteMatch
  const accidental = accidentalRaw === '♯' ? '#' : accidentalRaw === '♭' ? 'b' : accidentalRaw
  const pitchClass = PITCH_CLASS[(letter + accidental).toLowerCase()]
  if (pitchClass === undefined) return null

  // Pas de suffixe de mode = majeur, convention habituelle des tags (ex. TKEY "C").
  const isMinor = modeRaw !== undefined && (/^min/i.test(modeRaw) || modeRaw.toLowerCase() === 'm')
  return isMinor ? MINOR_CAMELOT[pitchClass] : MAJOR_CAMELOT[pitchClass]
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
// autres : repli sur le nom de fichier, artiste/durée/bpm/clé laissés vides.
export async function scanFolder(db: Database.Database, folderPath: string): Promise<Track[]> {
  const { parseFile } = await import('music-metadata')
  const files = await findAudioFiles(folderPath)

  const upsert = db.prepare(`
    INSERT INTO tracks (file_path, title, artist, duration_seconds, bpm, key_camelot)
    VALUES (@filePath, @title, @artist, @durationSeconds, @bpm, @keyCamelot)
    ON CONFLICT(file_path) DO UPDATE SET
      title = excluded.title,
      artist = excluded.artist,
      duration_seconds = excluded.duration_seconds,
      bpm = excluded.bpm,
      key_camelot = excluded.key_camelot
  `)

  for (const filePath of files) {
    let title = basename(filePath, extname(filePath))
    let artist: string | null = null
    let durationSeconds: number | null = null
    let bpm: number | null = null
    let keyCamelot: string | null = null

    try {
      const metadata = await parseFile(filePath, { duration: true })
      if (metadata.common.title) title = metadata.common.title
      if (metadata.common.artist) artist = metadata.common.artist
      if (metadata.format.duration) durationSeconds = metadata.format.duration
      if (metadata.common.bpm) bpm = metadata.common.bpm
      keyCamelot = toCamelotKey(metadata.common.key)
    } catch {
      // Tags illisibles ou fichier corrompu — on garde le repli ci-dessus et
      // on continue avec les fichiers suivants.
    }

    upsert.run({ filePath, title, artist, durationSeconds, bpm, keyCamelot })
  }

  return getAllTracks(db)
}
