# MixDeck — architecture.md

> Document de référence technique pour les agents de développement (Claude Code) et pour Julien.
> Dernière mise à jour : 2026-07-10
> Voir aussi : `roadmap.md`, `decision.md`, `progress.md`

## 1. Résumé

MixDeck est une application desktop macOS (Electron) simulant deux platines DJ : lecture indépendante sur deux decks, pitch, crossfader, filtre par voie, et chaîne d'effets basée sur de vrais plugins VST3/Audio Unit. Le design UI a été produit en parallèle avec Claude Design (export HTML) — voir §6.

Point structurant : l'hébergement de vrais plugins impose un **moteur audio natif en C++/JUCE**, Electron ne servant que de coquille d'interface. Détail complet des arbitrages dans `decision.md`.

## 2. Structure de dépôt proposée

```
mixdeck/
├── apps/
│   └── electron-ui/            # Interface Electron + React + TypeScript
│       ├── src/
│       │   ├── components/     # Deck, Crossfader, FilterKnob, PluginRack, LibraryBrowser...
│       │   ├── state/          # Store (paramètres UI, statut de connexion au moteur)
│       │   └── bridge/         # Wrapper JS autour du module natif (bridge.node)
│       └── package.json
├── native/
│   └── engine/                  # Moteur audio C++ / JUCE
│       ├── src/
│       │   ├── Deck.cpp/.h
│       │   ├── Mixer.cpp/.h
│       │   ├── FilterDSP.cpp/.h
│       │   ├── TimeStretch.cpp/.h
│       │   ├── PluginHost.cpp/.h
│       │   └── NodeBinding.cpp  # Point d'entrée N-API (node-addon-api)
│       ├── CMakeLists.txt
│       └── binding.gyp
├── db/
│   └── schema.sql               # Bibliothèque de sons (SQLite)
├── design/
│   └── export-html/             # Export Claude Design — référence visuelle et composants
├── docs/
│   ├── architecture.md
│   ├── roadmap.md
│   ├── decision.md
│   └── progress.md
└── package.json
```

## 3. Couches applicatives

| Couche | Techno | Responsabilité |
|---|---|---|
| Interface | Electron, React, TypeScript | Rendu (waveform, jog wheels, faders, knobs), aucun traitement du signal |
| Pont | node-addon-api (N-API) | Transmet les paramètres de contrôle en continu sans bloquer le thread audio |
| Moteur audio | C++ / JUCE | Lecture, pitch/time-stretch, filtre, mixage, plugin host — thread temps réel dédié |
| Hébergement plugins | JUCE AudioPluginFormatManager | Charge et exécute les plugins VST3/AU choisis, idéalement isolés hors-process |
| Bibliothèque | SQLite (better-sqlite3) | Métadonnées, tags, crates, cue points |

## 4. Composants fonctionnels

### 4.1 Decks (x2)
Lecture via `AudioFormatManager` (WAV, MP3, FLAC, AIFF), waveform précalculée à l'import, jog wheel virtuel (scratch/nudge), cue points, boucles.

### 4.2 Pitch / tempo
Slider ±8/16/50 %. Deux modes : vitesse liée (resampling simple) et time-stretch indépendant (Rubber Band ou SoundTouch). BPM détecté à l'import, sync automatique entre decks.

### 4.3 Fader / crossfader
Gain par deck (linéaire/log), crossfader avec courbes sélectionnables (linéaire, smooth, cut). Calcul de gain entièrement côté moteur natif.

### 4.4 Filtre
Filtre résonant multimode par deck (passe-bas/passe-haut), knob unique centré = neutre. DSP natif JUCE, latence non perceptible.

### 4.5 Plugins VST3 / Audio Unit
- Scan automatique des dossiers standards (`~/Library/Audio/Plug-Ins/VST3`, `/Components`).
- **Sélection manuelle du chemin** d'un plugin (bouton "Parcourir" ou glisser-déposer), en plus du scan automatique — pour les plugins hors dossiers standards.
- Chaîne d'effets par deck (et bus master), ajout/retrait/réordonnancement.
- Formats supportés en V1 : **VST3 et Audio Unit uniquement** (voir `decision.md` ADR-003 pour l'arbitrage VST2/FST).
- Hébergement hors-process recommandé pour la stabilité (au moins sur Windows ; sur macOS l'AU tourne généralement dans le process principal).

### 4.6 Bibliothèque de sons
SQLite local, scan/import de dossiers, tags, crates, cue points — cohérent avec les habitudes de l'app de bibliothèque existante (projet non réutilisé, mais logique similaire).

## 5. Contraintes non-fonctionnelles

| Exigence | Cible |
|---|---|
| Latence audio | < 10 ms (buffer réglable) |
| Stabilité | Un plugin tiers instable ne doit jamais faire planter l'UI/le process principal |
| Distribution | Usage strictement personnel, mono-utilisateur, mono-poste : **pas de notarization Apple ni de distribution**, build local uniquement (voir `decision.md` ADR-010) |
| Licences | Aucune dépendance à un SDK propriétaire non redistribuable (VST3 SDK MIT depuis oct. 2025 ; éviter le VST2/FST en V1 pour cette raison — cf. `decision.md`) |

## 6. Intégration du design (Claude Design)

Le design UI a été produit avec Claude Design, exporté en HTML (`design/export-html/MixDeck-1b-standalone.html`) et analysé pour en extraire la structure exacte des composants. Thème sombre : fond `#0d0e11`, panneaux `#14161b`/`#16181d`/`#101012`, accent Deck A `#4fd1c5` (teal), accent Deck B `#f0955a` (orange).

**Structure de l'écran principal :**
- Barre de titre décorative (feux macOS factices) + titre de session.
- Colonne gauche (200px) : recherche + liste de crates (Toutes / Techno / House / Favoris).
- Ligne principale : **Deck A** — **console master** — **Deck B**, côte à côte.
- Bandeau FX sous les decks : onglets "FX · Deck A" / "FX · Deck B", 3 emplacements de plugins affichés en cartes.
- Table de bibliothèque : Titre / Artiste / BPM / Clé (notation Camelot) / Durée / boutons "→ A" / "→ B".
- Modale d'édition de plugin en overlay (voir plus bas).

**Composant Deck (partagé A/B, un seul composant paramétré par `accent`/`label`) :**
- En-tête : pastille de couleur + label, BPM effectif + clé affichés une fois une piste chargée (BPM recalculé en fonction du pitch).
- Titre/artiste de la piste, ou "Aucune piste chargée".
- Platine (104×104) tournante avec bras de lecture animé (angle lié à la position de lecture), agit aussi comme zone de scratch/nudge (`onPlatterDown`).
- Transport : temps écoulé/total, boutons CUE / Play / Pause.
- Pitch : slider horizontal ±8 %, drag horizontal.
- Waveform : 72 barres, 3 paliers de couleur (bleu/vert/rouge), partie déjà jouée atténuée en opacité.
- 8 hot cues en grille 4×2, pad coloré + halo lumineux une fois posé.
- Fader de volume vertical (34×104), drag vertical.
- Mini-panneau "MIXER" : knob FILTRE + 3 knobs EQ (LOW/MID/HIGH), tous en rotation ±50 mappée en angle.
- Chaîne d'effets : 3 emplacements, chacun affiche le nom du plugin ou "+ Charger un plugin", bouton bypass (B) et suppression (×). Libellé "VST3 / AU" — cohérent avec `decision.md` ADR-003.

**Console master (colonne centrale, 170px) :** vumètres A/B (dégradé rouge/jaune/vert), knobs Master / Booth / Cue Mix (rotatifs), crossfader horizontal (thumb 26×16, plage 0–100 %).

**Modale d'édition de plugin :** titre = nom du plugin, sous-titre "Plugin VST3 / Audio Unit", bypass + fermeture, grille de 6 knobs génériques ("Param 1"...). **Point ouvert** (à trancher en Epic 4, voir `decision.md`) : ce mock utilise des knobs génériques plutôt que l'interface native du plugin — à décider si MixDeck embarque la vraie fenêtre d'édition du plugin (fenêtre native JUCE, plus fidèle) ou une surface de contrôle générique par paramètres (plus simple à intégrer en React mais moins complète).

**Modèle de données bibliothèque (déduit du mock) :** piste = titre, artiste, bpm, clé (Camelot), durée, crates (tags multiples) — confirme le schéma `db/schema.sql` de la §4.6.

## 7. Références

- Document d'architecture initial détaillé : `MixDeck_Architecture.docx`
- Export design analysé : `design/export-html/MixDeck-1b-standalone.html`
- JUCE Framework — https://juce.com
- node-addon-api — bridge Electron/N-API
