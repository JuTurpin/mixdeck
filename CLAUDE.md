# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## État du projet

**Epic 1 ("moteur audio standalone JUCE"), Epic 2 ("intégration Electron") et Epic 3 ("pitch avancé & BPM") sont terminés.** Epic 1 : les 4 stories (lecture 2 pistes, Mixer/crossfader, filtre résonant, pitch vitesse liée) validées à l'oreille. Avant l'Epic 2, une revue d'architecture a affiné le découpage (ADR-013 à ADR-016 : Engine API, couche Controller, modèle événementiel, machine d'état des Decks — voir `docs/decision.md`). Epic 2 : chaîne complète React → Controller → IPC → Bridge → moteur JUCE opérationnelle, vraie UI (Deck/Mixer/Filtre, calquée sur l'export Claude Design), fenêtre sans cadre natif (`frame: false`, fermer quitte toute l'app — ADR-010), modèle événementiel (ADR-015) poussé par le process principal, et checklist de robustesse/performance (Story 2.7) passée sans souci notable. Epic 3 : time-stretch indépendant (SoundTouch, ADR-006), détection BPM à l'import (`BpmAnalyzer`, BPM effectif affiché dans l'en-tête du Deck) et synchronisation automatique entre decks — un bouton Sync par deck en **verrou continu** (pas ponctuel, revu en cours de route sur demande de Julien après test) : tant qu'il est actif, tout changement de pitch/BPM de l'autre deck réapplique le pitch de celui-ci ; reprendre la main sur le slider désengage le verrou. Ce verrou continu n'existe que côté Electron — le harnais standalone JUCE garde un Sync ponctuel (écart assumé). **Toutes les stories validées par Julien.** Epic 4 (plugins VST3/AU) en cours : Story 4.0 (spike technique d'incrustation d'une vue native JUCE dans Electron via `Component::addToDesktop(0, hostWindowHandle)` + `juce::ScopedJuceInitialiser_GUI` en membre de `NativeEngine` — spike retiré depuis, rôle repris par la vraie chaîne d'effets), Story 4.1 (scan automatique VST3/AU, `PluginHost` — `AudioPluginFormatManager`/`KnownPluginList`, flags `JUCE_PLUGINHOST_VST3`/`JUCE_PLUGINHOST_AU`), Story 4.2 (sélection manuelle du chemin, VST3 uniquement — `PluginHost::addPluginFromPath()`, bouton "Parcourir" + glisser-déposer) et Story 4.3 (chaîne d'effets par deck + bus master) **validées**. Story 4.3 : instanciation réelle (`PluginHost::instantiatePlugin`), traitement temps réel lock-free (`PluginChain`, `native/engine/src/PluginChain.h/.cpp` — snapshot `std::atomic<const ChainSnapshot*>` ; le `std::atomic<std::shared_ptr<T>>` visé initialement n'est pas implémenté par le libc++ d'Apple malgré le C++20, static_assert à la compilation), bus master (`MasterBus.h/.cpp`), édition via GUI native incrustée (ADR-011) réutilisant le mécanisme de la 4.0. Correctif post-validation : une vue native incrustée se compose au-dessus de tout le DOM Electron indépendamment du z-index CSS, donc un bouton "réduire" dessiné en React devient inatteignable dès qu'un plugin la recouvre — `PluginChain::EditorHost` ajoute un en-tête natif (même hiérarchie de vues que l'éditeur, toujours cliquable) avec son propre bouton réduire. UI Electron : `PluginChainPanel.tsx` (composant partagé, ajout/retrait/réordonnancement/bypass/éditeur), intégré par deck (`Deck.tsx`) et pour le bus master (`ConsoleMaster.tsx`). Voir `docs/progress.md` pour l'état story par story à jour. Prochaine étape : Story 4.4 (isolation hors-process). Dépôt git initialisé (branche `main`), poussé en privé sur `github.com/JuTurpin/mixdeck` ; JUCE et SoundTouch sont vendorisés en submodules (`native/engine/JUCE`, `native/engine/SoundTouch`).

## Commandes (moteur natif `native/engine/`)

Harnais de test JUCE standalone (Epic 1) :
```
brew install cmake ninja        # une fois, si absents
git submodule update --init --recursive
cmake -S native/engine -B native/engine/build -G Ninja
cmake --build native/engine/build
open "native/engine/build/MixDeckStandalone_artefacts/Release/MixDeck Standalone.app"
```

Bridge N-API (Story 2.2, `native/engine/package.json`) — même dossier `build/`, configuré via cmake-js plutôt que cmake directement :
```
cd native/engine
npm install                     # node-addon-api + cmake-js, épinglés (ADR-012)
npx cmake-js compile
node test-bridge.js /chemin/vers/a.wav /chemin/vers/b.wav   # validation manuelle sans Electron/UI
```
Le module compilé est `native/engine/build/Release/mixdeck_bridge.node`, buildé contre le Node système.

**Piège** : si `native/engine/build/` (ou tout autre dossier de sortie cmake-js) a déjà été configuré par un `cmake` direct (sans cmake-js), relancer `cmake-js compile` dessus ne repasse pas par sa propre étape de configuration (il saute droit au build) et les variables `CMAKE_JS_*` ne sont jamais injectées → le target `mixdeck_bridge` n'est pas créé, silencieusement. Supprimer le dossier de build avant de rebasculer d'un mode de build à l'autre (ou utiliser `--out` pour un dossier séparé, voir ci-dessous).

Electron (Story 2.3, `apps/electron-ui/`) :
```
cd apps/electron-ui
npm install
npm run dev
```
Le Bridge doit être reconstruit contre l'ABI Node **d'Electron** (différente de celle du Node système) pour se charger dans le process principal Electron :
```
cd native/engine
npx cmake-js compile --out build-electron --runtime=electron --runtime-version=<version exacte d'electron, voir apps/electron-ui/package.json> --arch=arm64
```
Sortie dans `native/engine/build-electron/Release/mixdeck_bridge.node`, dossier séparé de `native/engine/build/` (Node système) pour ne pas mélanger les deux ABI — voir le piège ci-dessus.

Depuis la Story 2.4, `src/main/index.ts` garde une instance persistante du moteur et relaie les 13 méthodes de l'Engine API via `ipcMain.handle('mixdeck:<method>', ...)` ; `src/preload/index.ts` les expose au renderer via `contextBridge.exposeInMainWorld('mixdeck', ...)`. Les `Controller` (`src/renderer/src/controllers/`) enveloppent `window.mixdeck` — toute nouvelle logique métier doit vivre là, jamais dans le relai IPC ni dans le preload (ADR-014).

Depuis la Story 2.6, `src/main/engineEvents.ts` pousse en plus l'état/la position au renderer (`webContents.send`, ~200 ms) au lieu que celui-ci les demande — voir `src/shared/events.ts` (type `MixdeckEvent`) et `preload.onEvent()`. `useDeckState` s'y abonne ; ne pas réintroduire de polling côté renderer pour ces valeurs.

Il n'y a pas de package.json/workspace racine — `native/engine/` et `apps/electron-ui/` restent deux paquets npm indépendants. Pas de suite de tests automatisés pour l'instant — la validation de chaque story est manuelle (écoute au casque ou vérification visuelle de la fenêtre, voir `docs/progress.md`).

## Documentation de référence — à lire avant toute modification

Toutes les décisions et le cadrage vivent dans `docs/` et se référencent mutuellement :

- `docs/architecture.md` — architecture technique cible, structure de dépôt proposée, couches applicatives, composants fonctionnels, specs UI extraites du design (§6).
- `docs/decision.md` — journal des décisions (ADR-001 à ADR-016), avec alternatives écartées et statut (Accepté / Ouvert).
- `docs/roadmap.md` — séquencement en 6 epics selon le processus BMAD (Analysis → Planning → Solutioning → Implementation).
- `docs/progress.md` — tracker d'avancement **vivant**, à mettre à jour après chaque story.
- `design/export-html/` — exports Claude Design (`MixDeck-1b-standalone.html`, `deck_component.html`), référence visuelle exacte des composants (Deck, console master, modale plugin).
- `docs/MixDeck_Architecture.docx` — document d'architecture initial détaillé (antérieur aux ADR, utile pour le contexte narratif complet).
- `docs/sbom.json` — SBOM (CycloneDX) des dépendances externes, voir « Politique dépendances/sécurité » plus bas.

**Avant de coder une story**, relire les trois documents `architecture.md` / `decision.md` / `roadmap.md` en même temps : une décision (ADR) peut modifier une story de la roadmap sans que celle-ci soit reformulée ailleurs.

## Politique dépendances/sécurité (ADR-012)

Toute dépendance externe (submodule C++, lib vendorisée, package npm) est **épinglée à une version/commit exact** (jamais de plage, jamais `latest`/`main`). Chaque ajout ou mise à jour de dépendance doit être répercuté dans `docs/sbom.json` (CycloneDX). Avant de monter la version d'une dépendance, vérifier ses security advisories (GitHub Security Advisories pour les submodules C++, `npm audit`/Dependabot côté npm). En cas de vulnérabilité détectée, ne patcher **que** la dépendance concernée — jamais de mise à jour groupée ou de refactor opportuniste des autres dépendances à cette occasion.

## Vue d'ensemble du produit

MixDeck est une application desktop macOS (Electron) simulant deux platines DJ : lecture indépendante sur deux decks, pitch, crossfader, filtre par voie, et chaîne d'effets basée sur de **vrais plugins VST3/Audio Unit** tiers installés sur la machine. Usage strictement personnel, mono-utilisateur, mono-poste (pas de distribution).

## Décision architecturale structurante

Héberger de vrais plugins binaires VST3/AU est impossible en Web Audio API pur. Cela impose :

- **Moteur audio natif C++/JUCE** — lecture, pitch/time-stretch, filtre, mixage, plugin host, sur un thread temps réel dédié. Electron est une pure coquille d'interface (React/TypeScript), sans aucun traitement de signal.
- **Flux de commande à 4 couches** (ADR-014) : `React → Controller → Bridge → JUCE`. Le Controller (TS, côté `apps/electron-ui`) porte toute la logique métier ; le Bridge (`node-addon-api`) ne fait que traduire JS ↔ C++, **aucune logique métier dans le Bridge**.
- **Pont N-API** (`node-addon-api`) entre l'UI et le moteur natif — doit transmettre les paramètres de contrôle en continu **sans jamais bloquer le thread audio**. C'est le point de vigilance principal du projet pour éviter tout glitch audio.
- **Flux d'événements retour** (ADR-015) : `JUCE → Bridge → EventBus/Store → React`, avec agrégation/throttling des événements haute fréquence (position, vumètres).
- Le calcul de gain (fader/crossfader) se fait entièrement côté moteur natif, jamais côté UI.
- **Contrat "Engine API"** (ADR-013, détail dans `architecture.md` §4.7) défini avant tout code Bridge/UI — React ne doit jamais dépendre directement de l'implémentation du moteur.

## Structure de dépôt

```
mixdeck/
├── apps/electron-ui/       # Electron + React + TS (electron-vite, Story 2.3)
│   ├── src/shared/         # events.ts — type MixdeckEvent partagé main/preload/renderer (Story 2.6)
│   ├── src/main/           # fenêtre (sans cadre natif, Story 2.5) + relai IPC vers le Bridge (Story 2.4) + commandes fenêtre + engineEvents.ts (pump événementiel, Story 2.6)
│   ├── src/preload/        # contextBridge expose window.mixdeck (méthodes Engine API Deck/Mixer/Plugins + pickFile/pickPluginFile + commandes fenêtre + onEvent)
│   ├── src/renderer/src/   # controllers/ (DeckController/MixerController/PluginController/MasterBusController), state/ (useDeckState, événementiel depuis 2.6), components/ (TitleBar/Deck/FilterKnob/Crossfader/ConsoleMaster, Story 2.5 ; PluginChainPanel, Story 4.3) — bridge/ encore vide
│   └── package.json        # electron, react, electron-vite, vite... versions exactes (ADR-012)
├── native/engine/          # Moteur audio C++/JUCE
│   ├── JUCE/               # submodule, pinné (voir docs/sbom.json)
│   ├── src/                # Deck.cpp (1.1 + 1.4 pitchResampler + 2.1 DeckState/unloadTrack/pause/seek), Mixer.cpp (1.2), FilterDSP.cpp (1.3), Engine.cpp (2.2, graphe audio headless), NodeBinding.cpp (2.2, Bridge N-API) — TimeStretch/BpmAnalyzer (Epic 3), PluginHost/PluginChain/MasterBus (Epic 4) ajoutés depuis
│   ├── standalone-app/     # harnais de test JUCE (GUI minimale, hors Electron) — Epic 1 uniquement
│   ├── package.json        # deps du Bridge (node-addon-api, cmake-js)
│   └── CMakeLists.txt
├── db/                     # (Epic 5, pas encore créé) SQLite
├── design/export-html/     # Exports Claude Design — référence visuelle
└── docs/                   # architecture.md, roadmap.md, decision.md, progress.md, sbom.json
```

Pas de `package.json`/workspace racine : `native/engine/` et `apps/electron-ui/` restent deux paquets npm indépendants.

## Décisions à connaître avant d'implémenter (voir `decision.md` pour le détail complet)

- **Formats plugins V1 : VST3 + Audio Unit uniquement** (ADR-003). VST2/FST explicitement repoussé en backlog (Epic 4.5) — ne pas l'implémenter sans qu'on en discute, question de licence (GPL/statut légal flou du SDK VST2).
- **Chargement de plugin : scan automatique ET sélection manuelle du chemin** (ADR-004) — l'UI doit toujours proposer un "Parcourir"/glisser-déposer, pas seulement le scan des dossiers standards.
- **Isolation hors-process des plugins**, au moins sur Windows ; sur macOS l'AU reste généralement dans le process principal (ADR-005).
- **Pas de notarization/distribution Apple** (ADR-010) : build et signature ad-hoc locale uniquement (`codesign --sign -`). Ne jamais ajouter de logique de notarization, Developer ID ou Mac App Store.
- **Projet neuf, sans réutilisation de code** d'une app de bibliothèque existante (ADR-009) — logique similaire (SQLite local) mais pas de code partagé.
- **Dépendances épinglées + SBOM à jour** (ADR-012) — voir « Politique dépendances/sécurité » plus haut.
- **Engine API** (ADR-013) — contrat Deck/Mixer/Plugins/Monitoring (`architecture.md` §4.7). Deck et Mixer sont couverts (Story 2.1) ; Plugins couvert depuis la Story 4.3 (`PluginChain`, par deck et bus master) ; Monitoring (`getPeakMeter`/`getWaveform`) différé à 2.5/2.6 (nouveau DSP requis), toujours pas implémenté.
- **Couche Controller entre React et le Bridge** (ADR-014) — `React → Controller → Bridge → JUCE`, Bridge = traduction pure.
- **Modèle événementiel moteur → UI** (ADR-015) — `JUCE → Bridge → EventBus/Store → React`, événements agrégés/throttlés pour position/vumètres. Implémenté en Story 2.6 sans callback natif C++ (`Napi::ThreadSafeFunction`) : le process principal lit l'état directement sur `nativeEngine` (déjà dans ce process, pas d'IPC) et pousse via `webContents.send` — voir `src/main/engineEvents.ts`. À revisiter si un vrai callback natif devient nécessaire (Epic 3/4).
- **Machine d'état des Decks** (ADR-016) — `EMPTY/LOADING/READY/PLAYING/PAUSED/STOPPED/ERROR`, seule source de vérité pour l'UI (pas de booléens épars côté React).
- **UI Story 2.5 volontairement partielle** : le style Claude Design est repris pour Deck/Mixer/Filtre uniquement (ce que le moteur pilote réellement) — à l'origine, BPM/clé, waveform, hot cues, EQ 3 bandes, chaîne d'effets, vumètres, bibliothèque/crates étaient **absents de l'écran**, pas des placeholders, ajoutés story par story au fur et à mesure que leur moteur existe (BPM en 3.2, chaîne d'effets en 4.3) plutôt qu'en avance. Restent absents : clé, waveform, hot cues, EQ 3 bandes, vumètres, bibliothèque/crates.
- **ADR-006** (bibliothèque de time-stretch) tranchée : SoundTouch (LGPL-2.1-or-later), vendorisée en submodule comme JUCE (`native/engine/SoundTouch`, pinné au tag 2.4.1) — voir `TimeStretch.cpp/.h` (Story 3.1).
- **ADR-011** (interface d'édition des plugins) tranchée : GUI native du plugin (fenêtre native JUCE embarquant l'éditeur réel), choix de Julien. Implique de lier la variante GUI de `juce_audio_processors` côté Bridge Electron (aujourd'hui seule la variante headless est liée dans `engine-core`, partagée avec le Bridge Node sans fenêtre) et de résoudre l'incrustation d'une fenêtre/vue native JUCE dans Electron — à cadrer avant/pendant la story 4.3.
- Toutes les décisions ADR sont désormais tranchées (Accepté) — plus aucune décision ouverte en attente avant de coder une story.

## Bonnes pratiques temps réel (mémoire/ownership)

Applicable à tout code natif futur (Epic 2 et au-delà), pas seulement lors de l'écriture initiale : pas d'allocation dans le thread audio, buffers préalloués, pas de copies inutiles, RAII/`std::unique_ptr`, charger les ressources avant lecture, privilégier des échanges lock-free entre UI et moteur. Réfléchir explicitement à l'ownership des objets (qui crée/détruit les Decks, plugins, buffers) avant d'écrire le code, pas après.

Garde-fous Epic 2 (revue d'architecture du 2026-07-11) : ne pas changer les choix techniques déjà actés (JUCE/Electron/React/N-API/SQLite) ; ne pas réécrire les composants Epic 1 déjà validés sans raison ; ne pas ajouter de dépendance inutile (cohérent avec ADR-012) ; ne jamais mettre de logique métier dans le Bridge (ADR-014) ; ne pas modifier les epics suivants (3 à 6) à l'occasion d'un travail sur l'Epic 2.

## Processus de développement (BMAD)

Le développement suit les 4 phases BMAD, avec Claude Code écrivant tout le code (UI, moteur C++/JUCE, bindings). Restent manuelles côté Julien : compilation/tests sur machine réelle, et évaluation à l'oreille de la qualité audio — ne jamais prétendre avoir validé un rendu audio sans que Julien l'ait écouté.

**Règle impérative : plan avant code.** Avant toute production de code pour une story (ou tout changement non trivial), établir d'abord un plan avec Julien (mode plan) et attendre sa validation explicite. Tant que ce plan n'est pas défini et approuvé, ne pas écrire de code. Cette règle s'applique systématiquement, story après story — ne pas enchaîner directement sur l'implémentation même si une story précédente vient d'être validée.

Boucle par story : `bmad-create-story` → `bmad-dev-story` → `bmad-code-review`, puis mise à jour de `progress.md`. `bmad-retrospective` à la fin de chaque epic.

Ordre de dépendance des epics :
```
Epic 1 (moteur standalone JUCE, hors Electron)
   └─► Epic 2 (intégration Electron / bridge N-API / UI React)
          ├─► Epic 3 (pitch avancé, time-stretch, BPM)      ─┐
          ├─► Epic 4 (plugins VST3/AU)                       ├─► Epic 6 (build local)
          └─► Epic 5 (bibliothèque SQLite)                  ─┘
```

**Prochaine action (voir `progress.md`)** : Epic 4 (plugins VST3/AU), Story 4.3 (chaîne d'effets par deck + bus master) validée — passer à la Story 4.4 (isolation hors-process).

## Contraintes non-fonctionnelles

| Exigence | Cible |
|---|---|
| Latence audio | < 10 ms (buffer réglable) |
| Stabilité | Un plugin tiers instable ne doit jamais faire planter l'UI/le process principal |
| Licences | Pas de dépendance à un SDK propriétaire non redistribuable (VST3 SDK MIT depuis oct. 2025 ; VST2/FST évité en V1 pour cette raison) |
