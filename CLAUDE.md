# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## État du projet

Epic 1 ("moteur audio standalone JUCE, hors Electron") est en cours — Stories 1.1 (lecture 2 pistes), 1.2 (Mixer : gain par deck + crossfader Linéaire/Smooth/Cut) et 1.3 (filtre résonant multimode par deck) codées, compilées et **validées à l'oreille par Julien**. Voir `docs/progress.md` pour l'état story par story à jour. Dépôt git initialisé (branche `main`), poussé en privé sur `github.com/JuTurpin/mixdeck` ; JUCE est vendorisé en submodule dans `native/engine/JUCE`.

## Commandes (moteur natif `native/engine/`)

```
brew install cmake ninja        # une fois, si absents
git submodule update --init --recursive
cmake -S native/engine -B native/engine/build -G Ninja
cmake --build native/engine/build
open "native/engine/build/MixDeckStandalone_artefacts/MixDeck Standalone.app"
```

Il n'y a pas encore de package.json/npm racine (Electron arrive en Epic 2) : tout le build actuel passe par CMake. Pas de suite de tests automatisés pour l'instant — la validation de chaque story est manuelle (écoute au casque, voir `docs/progress.md`).

## Documentation de référence — à lire avant toute modification

Toutes les décisions et le cadrage vivent dans `docs/` et se référencent mutuellement :

- `docs/architecture.md` — architecture technique cible, structure de dépôt proposée, couches applicatives, composants fonctionnels, specs UI extraites du design (§6).
- `docs/decision.md` — journal des décisions (ADR-001 à ADR-012), avec alternatives écartées et statut (Accepté / Ouvert).
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
- **Pont N-API** (`node-addon-api`) entre l'UI et le moteur natif — doit transmettre les paramètres de contrôle en continu **sans jamais bloquer le thread audio**. C'est le point de vigilance principal du projet pour éviter tout glitch audio.
- Le calcul de gain (fader/crossfader) se fait entièrement côté moteur natif, jamais côté UI.

## Structure de dépôt

```
mixdeck/
├── apps/electron-ui/       # (Epic 2, pas encore créé) Electron + React + TS
├── native/engine/          # Moteur audio C++/JUCE
│   ├── JUCE/               # submodule, pinné (voir docs/sbom.json)
│   ├── src/                # Deck.cpp (1.1), Mixer.cpp (1.2, calcul de gain seul, pas d'AudioSource), FilterDSP.cpp (1.3, StateVariableTPTFilter) — TimeStretch/PluginHost/NodeBinding ajoutés au fil des stories suivantes
│   ├── standalone-app/     # harnais de test JUCE (GUI minimale, hors Electron) — Epic 1 uniquement
│   └── CMakeLists.txt
├── db/                     # (Epic 5, pas encore créé) SQLite
├── design/export-html/     # Exports Claude Design — référence visuelle
└── docs/                   # architecture.md, roadmap.md, decision.md, progress.md, sbom.json
```

Pas de `package.json` racine tant qu'Electron/npm n'entrent pas en jeu (Epic 2) — ne pas en créer un par anticipation.

## Décisions à connaître avant d'implémenter (voir `decision.md` pour le détail complet)

- **Formats plugins V1 : VST3 + Audio Unit uniquement** (ADR-003). VST2/FST explicitement repoussé en backlog (Epic 4.5) — ne pas l'implémenter sans qu'on en discute, question de licence (GPL/statut légal flou du SDK VST2).
- **Chargement de plugin : scan automatique ET sélection manuelle du chemin** (ADR-004) — l'UI doit toujours proposer un "Parcourir"/glisser-déposer, pas seulement le scan des dossiers standards.
- **Isolation hors-process des plugins**, au moins sur Windows ; sur macOS l'AU reste généralement dans le process principal (ADR-005).
- **Pas de notarization/distribution Apple** (ADR-010) : build et signature ad-hoc locale uniquement (`codesign --sign -`). Ne jamais ajouter de logique de notarization, Developer ID ou Mac App Store.
- **Projet neuf, sans réutilisation de code** d'une app de bibliothèque existante (ADR-009) — logique similaire (SQLite local) mais pas de code partagé.
- **Dépendances épinglées + SBOM à jour** (ADR-012) — voir « Politique dépendances/sécurité » plus haut.
- Deux décisions **encore ouvertes**, à trancher avant de coder les stories concernées :
  - ADR-006 (lib de time-stretch Rubber Band vs SoundTouch, licence à confirmer) — avant Epic 3.
  - ADR-011 (GUI native du plugin vs knobs génériques pour l'éditeur de plugin) — avant la story 4.3.

## Processus de développement (BMAD)

Le développement suit les 4 phases BMAD, avec Claude Code écrivant tout le code (UI, moteur C++/JUCE, bindings). Restent manuelles côté Julien : compilation/tests sur machine réelle, et évaluation à l'oreille de la qualité audio — ne jamais prétendre avoir validé un rendu audio sans que Julien l'ait écouté.

Boucle par story : `bmad-create-story` → `bmad-dev-story` → `bmad-code-review`, puis mise à jour de `progress.md`. `bmad-retrospective` à la fin de chaque epic.

Ordre de dépendance des epics :
```
Epic 1 (moteur standalone JUCE, hors Electron)
   └─► Epic 2 (intégration Electron / bridge N-API / UI React)
          ├─► Epic 3 (pitch avancé, time-stretch, BPM)      ─┐
          ├─► Epic 4 (plugins VST3/AU)                       ├─► Epic 6 (build local)
          └─► Epic 5 (bibliothèque SQLite)                  ─┘
```

**Prochaine action (voir `progress.md`)** : Story 1.4 (pitch "vitesse liée", resampling simple).

## Contraintes non-fonctionnelles

| Exigence | Cible |
|---|---|
| Latence audio | < 10 ms (buffer réglable) |
| Stabilité | Un plugin tiers instable ne doit jamais faire planter l'UI/le process principal |
| Licences | Pas de dépendance à un SDK propriétaire non redistribuable (VST3 SDK MIT depuis oct. 2025 ; VST2/FST évité en V1 pour cette raison) |
