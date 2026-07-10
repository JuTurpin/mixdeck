# MixDeck — progress.md

> Tracker d'avancement vivant. À mettre à jour après chaque story (voir `roadmap.md`).
> Dernière mise à jour : 2026-07-10

## Statut global

**Phase en cours : Implementation — Epic 1, Story 1.2 ✅ validée**
Dépôt git initialisé (branche `main`), poussé en privé sur GitHub (`JuTurpin/mixdeck`). JUCE 8.0.14 vendorisé en submodule (`native/engine/JUCE`, commit `2cdfca8f`). Story 1.2 (Mixer : gain par deck + crossfader linéaire/smooth/cut) codée, compile et **validée à l'oreille par Julien** (ADR-008). Prochaine étape : Story 1.3 (filtre résonant).

## Suivi par epic / story

| Epic | Story | Statut | Notes |
|---|---|---|---|
| 0 — Cadrage | 0.1 Cadrage fonctionnel | ✅ Fait | Voir `architecture.md`, `decision.md` |
| 0 — Cadrage | 0.2 Intégrer l'export Claude Design | ✅ Fait | Specs extraites dans `architecture.md` §6 |
| 1 — Moteur standalone | 1.1 Lecture 2 pistes | ✅ Fait | `native/engine/` (Deck, MainComponent, DeckPanel). Validée au casque par Julien : lecture simultanée de 2 pistes OK. |
| 1 — Moteur standalone | 1.2 Mixer + crossfader | ✅ Fait | `Mixer` (gain calc, pas d'AudioSource) + `Deck::setGain`. Courbes Linéaire/Smooth/Cut validées au casque. |
| 1 — Moteur standalone | 1.3 Filtre résonant | ⬜ À faire | |
| 1 — Moteur standalone | 1.4 Pitch vitesse liée | ⬜ À faire | |
| 2 — Intégration Electron | 2.1 Bridge N-API | ⬜ À faire | Dépend d'Epic 1 |
| 2 — Intégration Electron | 2.2 UI React 2 decks | ⬜ À faire | Peut démarrer, design dispo |
| 2 — Intégration Electron | 2.3 Faders/crossfader/filtre connectés | ⬜ À faire | |
| 3 — Pitch avancé | 3.1 Time-stretch (lib à choisir) | ⬜ À faire | Décision licence ouverte (ADR-006) |
| 3 — Pitch avancé | 3.2 Détection BPM | ⬜ À faire | |
| 3 — Pitch avancé | 3.3 Sync automatique | ⬜ À faire | |
| 4 — Plugins VST3/AU | 4.0 GUI native vs knobs génériques | ⬜ Ouvert | Décision à prendre — ADR-011 |
| 4 — Plugins VST3/AU | 4.1 Scan automatique | ⬜ À faire | |
| 4 — Plugins VST3/AU | 4.2 Sélection manuelle du chemin | ⬜ À faire | |
| 4 — Plugins VST3/AU | 4.3 Chaîne d'effets par deck | ⬜ À faire | Dépend de 4.0 |
| 4 — Plugins VST3/AU | 4.4 Isolation hors-process | ⬜ À faire | |
| 4 — Plugins VST3/AU | 4.5 Support VST2/FST (backlog) | ⬜ Backlog | Non priorisé — cf. ADR-003 |
| 5 — Bibliothèque | 5.1 Base SQLite | ⬜ À faire | |
| 5 — Bibliothèque | 5.2 Tags/crates/cue points | ⬜ À faire | |
| 6 — Build local | 6.1 Build combiné electron-builder/CMake | ⬜ À faire | |
| 6 — Build local | 6.2 Signature ad-hoc locale | ⬜ À faire | Pas de notarization — usage perso (ADR-010) |
| 6 — Build local | 6.3 Script de build/lancement | ⬜ À faire | |

## Prochaine action

Démarrer la Story 1.3 (filtre résonant multimode par deck).

## Journal des mises à jour

- **2026-07-10** — Création des documents `architecture.md`, `roadmap.md`, `decision.md`, `progress.md`. Aucun développement démarré.
- **2026-07-10** — Analyse de l'export Claude Design (`MixDeck-1b-standalone.html`) : Story 0.2 terminée, specs intégrées dans `architecture.md` §6. Suppression de la piste distribution/App Store (ADR-010) — Epic 6 simplifié en "Build local & lancement". Nouveau point ouvert ADR-011 (GUI plugin native vs générique).
- **2026-07-10** — Démarrage Story 1.1 : `git init`, réorganisation `Documentation ` → `docs/` + `design/`, ADR-012 (politique dépendances/SBOM, `docs/sbom.json`), JUCE 8.0.14 vendorisé en submodule (`native/engine/JUCE`, commit `2cdfca8f`). Code du moteur : `mixdeck::Deck` (AudioTransportSource + AudioFormatReaderSource), harnais de test `MixDeckStandalone` (JUCE GUI, 2 `DeckPanel` A/B, `MixerAudioSource`). Build CMake/Ninja OK, app lancée sans crash.
- **2026-07-10** — Story 1.1 validée à l'oreille par Julien (lecture simultanée de 2 pistes indépendantes, OK). Dépôt poussé en privé sur `github.com/JuTurpin/mixdeck`.
- **2026-07-10** — Story 1.2 : `mixdeck::Mixer` (calcul de gain seul, pas d'AudioSource) + `Deck::setGain` déléguant à `AudioTransportSource::setGain`. Courbes crossfader Linéaire/Smooth (constant-power)/Cut (scratch, ±8% aux extrêmes). UI harnais : slider volume par deck + crossfader + combo courbe. Validée à l'oreille par Julien (les 3 courbes se comportent comme attendu).
