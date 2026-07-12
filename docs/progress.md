# MixDeck — progress.md

> Tracker d'avancement vivant. À mettre à jour après chaque story (voir `roadmap.md`).
> Dernière mise à jour : 2026-07-11

## Statut global

**Epic 1 ✅ terminé — jalon "moteur standalone JUCE testable au casque" atteint**
Dépôt git initialisé (branche `main`), poussé en privé sur GitHub (`JuTurpin/mixdeck`). JUCE 8.0.14 vendorisé en submodule (`native/engine/JUCE`, commit `2cdfca8f`). Les 4 stories de l'Epic 1 (lecture 2 pistes, mixer/crossfader, filtre résonant, pitch vitesse liée) sont codées, compilent et **validées à l'oreille par Julien** (ADR-008).

**Epic 2 ✅ terminé — jalon "mixer deux morceaux depuis l'interface graphique" atteint**
Avant de démarrer l'Epic 2, révision d'architecture (ADR-013 à ADR-016 : Engine API, couche Controller, modèle événementiel, machine d'état des Decks) — Epic 2 redécoupé en 7 stories en conséquence (voir `roadmap.md`). Les 7 stories sont codées, compilent et **validées par Julien**, y compris la checklist de robustesse/performance de la Story 2.7 (RAS, seul point connu : ergonomie du drag de la platine, déjà noté dans la roadmap section Addon). Prochaine étape : Epic 3 (pitch avancé & BPM).

## Suivi par epic / story

| Epic | Story | Statut | Notes |
|---|---|---|---|
| 0 — Cadrage | 0.1 Cadrage fonctionnel | ✅ Fait | Voir `architecture.md`, `decision.md` |
| 0 — Cadrage | 0.2 Intégrer l'export Claude Design | ✅ Fait | Specs extraites dans `architecture.md` §6 |
| 1 — Moteur standalone | 1.1 Lecture 2 pistes | ✅ Fait | `native/engine/` (Deck, MainComponent, DeckPanel). Validée au casque par Julien : lecture simultanée de 2 pistes OK. |
| 1 — Moteur standalone | 1.2 Mixer + crossfader | ✅ Fait | `Mixer` (gain calc, pas d'AudioSource) + `Deck::setGain`. Courbes Linéaire/Smooth/Cut validées au casque. |
| 1 — Moteur standalone | 1.3 Filtre résonant | ✅ Fait | `FilterDSP` (StateVariableTPTFilter), knob unique -1..+1 par deck, centre = bypass réel. Validé au casque. |
| 1 — Moteur standalone | 1.4 Pitch vitesse liée | ✅ Fait | `Deck::setPitch` via `juce::ResamplingAudioSource` enveloppant `transportSource` (-50%..+50%). Validé au casque. |
| 2 — Intégration Electron | 2.1 Définition de l'Engine API | ✅ Fait | `Deck` : `DeckState` (ADR-016) + `unloadTrack()`/`pause()`/`seek()`. `Mixer` déjà conforme. Validé (+ non-régression Epic 1). |
| 2 — Intégration Electron | 2.2 Bridge N-API | ✅ Fait | `Engine` (headless) + `NodeBinding.cpp` (cmake-js). Validé via `test-bridge.js` : 2 pistes chargées/jouées depuis Node, sans fenêtre. |
| 2 — Intégration Electron | 2.3 Initialisation Electron | ✅ Fait | `apps/electron-ui` (electron-vite, React+TS). Bridge reconstruit pour l'ABI Electron (`build-electron/`). Fenêtre + diagnostic Bridge validés par Julien. |
| 2 — Intégration Electron | 2.4 Controllers | ✅ Fait | `DeckController`/`MixerController` + relai IPC (main/preload). État par polling temporaire (`useDeckState`). Validé par Julien : chargement, transport, volume, filtre, pitch, crossfader tous fonctionnels. |
| 2 — Intégration Electron | 2.5 Communication UI → Engine | ✅ Fait | Vraie UI (`Deck`/`Crossfader`/`ConsoleMaster`/`FilterKnob`/`TitleBar`), sélecteur de fichier natif, fenêtre sans cadre natif (fermer/réduire/agrandir via IPC). Validée par Julien. |
| 2 — Intégration Electron | 2.6 Communication Engine → UI | ✅ Fait | `engineEvents.ts` (pump process principal, 200ms, push via `webContents.send`), `onEvent` côté preload/renderer. `useDeckState` n'interroge plus le Bridge. Validé par Julien. |
| 2 — Intégration Electron | 2.7 Validation fonctionnelle et performances | ✅ Fait | Checklist robustesse/cycle de vie/performance parcourue par Julien : RAS. Seul point connu : ergonomie du drag de la platine (déjà en roadmap, Addon). **Epic 2 terminé.** |
| 3 — Pitch avancé | 3.1 Time-stretch (SoundTouch) | ✅ Fait | ADR-006 tranché. `TimeStretch`/`PitchMode` intégrés dans `Deck`, backend uniquement (pas de toggle React). Un bug de saccade initial (granularité par lots de SoundTouch mal absorbée) corrigé via un tampon de sécurité (~185ms). **Validé à l'oreille par Julien** via le harnais standalone (`DeckPanel`), y compris avec deux pistes en lecture simultanée et mouvements brusques sur les curseurs. |
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

Story 3.1 (time-stretch indépendant, SoundTouch) validée. Poursuivre l'Epic 3 avec la Story 3.2 (détection BPM + beat-grid), avec son propre plan dédié.

## Journal des mises à jour

- **2026-07-10** — Création des documents `architecture.md`, `roadmap.md`, `decision.md`, `progress.md`. Aucun développement démarré.
- **2026-07-10** — Analyse de l'export Claude Design (`MixDeck-1b-standalone.html`) : Story 0.2 terminée, specs intégrées dans `architecture.md` §6. Suppression de la piste distribution/App Store (ADR-010) — Epic 6 simplifié en "Build local & lancement". Nouveau point ouvert ADR-011 (GUI plugin native vs générique).
- **2026-07-10** — Démarrage Story 1.1 : `git init`, réorganisation `Documentation ` → `docs/` + `design/`, ADR-012 (politique dépendances/SBOM, `docs/sbom.json`), JUCE 8.0.14 vendorisé en submodule (`native/engine/JUCE`, commit `2cdfca8f`). Code du moteur : `mixdeck::Deck` (AudioTransportSource + AudioFormatReaderSource), harnais de test `MixDeckStandalone` (JUCE GUI, 2 `DeckPanel` A/B, `MixerAudioSource`). Build CMake/Ninja OK, app lancée sans crash.
- **2026-07-10** — Story 1.1 validée à l'oreille par Julien (lecture simultanée de 2 pistes indépendantes, OK). Dépôt poussé en privé sur `github.com/JuTurpin/mixdeck`.
- **2026-07-10** — Story 1.2 : `mixdeck::Mixer` (calcul de gain seul, pas d'AudioSource) + `Deck::setGain` déléguant à `AudioTransportSource::setGain`. Courbes crossfader Linéaire/Smooth (constant-power)/Cut (scratch, ±8% aux extrêmes). UI harnais : slider volume par deck + crossfader + combo courbe. Validée à l'oreille par Julien (les 3 courbes se comportent comme attendu).
- **2026-07-10** — Story 1.3 : `mixdeck::FilterDSP` (wrapper `juce::dsp::StateVariableTPTFilter`), knob unique -1..+1 par deck (zone morte centrale = bypass réel, gauche = passe-bas qui se ferme, droite = passe-haut qui s'ouvre, résonance croissante en fin de course). Intégré dans `Deck::getNextAudioBlock` après le gain. UI harnais : knob rotatif "Filtre" par deck. Validée à l'oreille par Julien (comportement nickel).
- **2026-07-10** — Story 1.4 : pitch "vitesse liée" via `juce::ResamplingAudioSource` (`pitchResampler`) enveloppant `transportSource` dans `Deck` ; `Deck::setPitch(percent)` -50..+50 % → ratio 1.0±0.5 clampé. UI harnais : slider "Pitch" par deck. **Epic 1 terminé** (jalon moteur standalone testable au casque atteint) — validé à l'oreille par Julien.
- **2026-07-11** — Revue d'architecture de Julien (`docs/update.md`, fusionné puis supprimé) avant démarrage de l'Epic 2 : ADR-013 (contrat Engine API), ADR-014 (couche Controller entre React et Bridge), ADR-015 (modèle événementiel moteur → UI), ADR-016 (machine d'état des Decks). Epic 2 redécoupé en 7 stories dans `roadmap.md`. Aucun changement au code Epic 1.
- **2026-07-11** — Story 2.1 : `DeckState` (ADR-016) + `Deck::unloadTrack()`/`pause()`/`seek()`, `getState()` détecte aussi la fin de piste naturelle. Harnais de test : boutons Pause/Unload, label d'état, slider de position (seek). `docs/architecture.md` §4.7 mis à jour avec les signatures finalisées. Validé par Julien : nouveaux contrôles OK **et** aucune régression sur les fonctionnalités Epic 1.
- **2026-07-11** — Story 2.2 : `mixdeck::Engine` (graphe audio headless, `native/engine/src/Engine.h/.cpp`) + `NodeBinding.cpp` (Bridge N-API, `node-addon-api`, traduction pure — ADR-014). Build via cmake-js (`native/engine/package.json`, deps épinglées, `docs/sbom.json` à jour). Validé par Julien via `test-bridge.js` : 2 pistes chargées/jouées depuis un script Node, sans fenêtre JUCE ; harnais standalone Epic 1 toujours fonctionnel.
- **2026-07-11** — Story 2.3 : `apps/electron-ui` scaffoldé (electron-vite, React+TS ; `vite@7.3.6`/`@vitejs/plugin-react@5.2.0` épinglés pour compatibilité peer avec `electron-vite@5.0.0`). `src/main/index.ts` ouvre la fenêtre et charge le Bridge (reconstruit pour l'ABI Electron dans `native/engine/build-electron/`) à titre de diagnostic. Écran minimal React (thème sombre). Validé par Julien : fenêtre ouverte, `Bridge OK, deck A state = EMPTY` dans les logs, fermeture propre.
- **2026-07-11** — Story 2.4 : relai IPC (`ipcMain.handle`/`contextBridge`) pour les 13 méthodes de l'Engine API, traduction pure (ADR-014). `DeckController`/`MixerController` côté renderer (`src/renderer/src/controllers/`). État rafraîchi par polling temporaire (`useDeckState`, ~300 ms — remplacé par le modèle événementiel en Story 2.6). UI de test minimale (inputs bruts) dans `App.tsx`. Validé par Julien : chargement d'un son par deck, fader, filtre, pitch, crossfader, états bien reflétés — chaîne React → Controller → IPC → Bridge → moteur entièrement opérationnelle.
- **2026-07-11** — Story 2.5 : vraie UI calquée sur l'export Claude Design, scope limité à Deck/Mixer/Filtre (BPM/clé, waveform, hot cues, EQ, effets, vumètres, bibliothèque absents — pas de moteur derrière, décision actée avec Julien). Nouveaux composants `TitleBar`/`Deck`/`FilterKnob`/`Crossfader`/`ConsoleMaster`. Sélecteur de fichier natif (`pickFile` + `DeckController.pickAndLoadTrack`). Corrections en cours de validation : fenêtre passée sans cadre natif (`frame: false`, la barre décorative faisait doublon avec la vraie), pastilles rouge/jaune/vert câblées (fermer = quitter l'app entièrement, réduire, agrandir/restaurer — usage mono-fenêtre, ADR-010). Ajout roadmap (section Addon) : ergonomie du drag de la platine à améliorer plus tard. Validé par Julien.
- **2026-07-11** — Story 2.6 : modèle événementiel (ADR-015) sans callback natif C++ — le process principal (`src/main/engineEvents.ts`) lit l'état directement sur `nativeEngine` (pas d'IPC, déjà dans ce process) toutes les 200 ms et pousse les changements via `webContents.send`, plutôt que le renderer ne les demande. Émet `TrackLoaded`/`PlaybackStarted`/`PlaybackPaused`/`PlaybackStopped`/`TrackEnded`/`EngineError` (transitions) + `PositionChanged` (en continu) ; `TrackEnded` vs `PlaybackStopped` distingué par heuristique (position proche de la durée). `preload` expose `onEvent()` ; `useDeckState` s'abonne au lieu de sonder — signature de retour inchangée, `Deck.tsx` intact. Aucun changement natif. Validé par Julien : réactivité conservée, transitions bien loguées, `TrackEnded` détecté correctement en fin de piste.
- **2026-07-11** — Story 2.7 : checklist de validation (robustesse/cas limites, cycle de vie fenêtre, performance) parcourue par Julien, sans code ajouté a priori. Résultat : RAS sur toute la checklist, seul point relevé déjà connu et noté en roadmap (ergonomie du drag de la platine). **Epic 2 terminé** — jalon "mixer deux morceaux depuis l'interface graphique" atteint et confirmé sous contrainte.
