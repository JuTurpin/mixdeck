# MixDeck — roadmap.md

> Séquençage du développement, basé sur les 4 phases du processus BMAD (Build More Architect Dreams — bmad-method.org) : **Analysis → Planning → Solutioning → Implementation**.
> Dernière mise à jour : 2026-07-11
> Voir aussi : `architecture.md`, `decision.md`, `progress.md`

## Comment lire ce document

Chaque Epic correspond à un lot de travail cohérent. Chaque story est une unité implémentable en une session avec Claude Code (`bmad-create-story` → `bmad-dev-story` → `bmad-code-review`). Le statut de chaque story est suivi dans `progress.md`, pas ici — ce fichier décrit la séquence cible, pas l'avancement réel.

---

## Phase 1 — Analysis & Planning (largement faite)

Cadrage fonctionnel déjà posé au fil des échanges (objectif produit, arbitrages VST3/AU vs VST2/FST, moteur natif vs Web Audio). Le design a été produit séparément avec Claude Design.

- **Story 0.1 — Cadrage fonctionnel** ✅ fait (voir `architecture.md` §1, `decision.md`)
- **Story 0.2 — Intégrer l'export Claude Design** ✅ fait — specs de composants extraites dans `architecture.md` §6 (structure de l'écran, composant Deck, console master, modale plugin)

## Phase 2 — Solutioning

Découpage en epics et stories, dans l'ordre de dépendance recommandé.

### Epic 1 — Moteur audio standalone (JUCE, hors Electron)
Objectif : valider le DSP à l'oreille avant d'investir dans le bridge Electron.
1.1 Lecture de fichier audio sur 2 pistes indépendantes
1.2 Mixer : gains par deck + crossfader (courbes linéaire / smooth / cut)
1.3 Filtre résonant multimode par deck
1.4 Pitch "vitesse liée" (resampling simple)
**Jalon** : application console/standalone JUCE, testable au casque.

### Epic 2 — Intégration Electron
> Découpage révisé le 2026-07-11 suite à la revue d'architecture de Julien (`decision.md` ADR-013 à ADR-016) — couche Controller, contrat Engine API et modèle événementiel posés avant le code UI/Bridge.

2.1 Définition de l'Engine API (ADR-013) — contrat Deck/Mixer/Plugins/Monitoring, mappé sur le `Deck`/`Mixer` C++ existant (Epic 1)
2.2 Bridge N-API (node-addon-api) — traduction pure JS ↔ C++, aucune logique métier (ADR-014)
2.3 Initialisation Electron (fenêtre, packaging de dev)
2.4 Controllers (ADR-014) — logique métier côté JS entre React et le Bridge
2.5 Communication UI → Engine — React/Controller pilotent Deck/Mixer/Filtre en temps réel, calqué sur l'export Claude Design
2.6 Communication Engine → UI — modèle événementiel (ADR-015) et machine d'état des Decks (ADR-016)
2.7 Validation fonctionnelle et performances
**Jalon** : mixer deux morceaux depuis l'interface graphique.
**Dépend de** : Epic 1, Story 0.2.

### Epic 3 — Pitch avancé & BPM
3.1 Intégration d'une bibliothèque de time-stretch (SoundTouch, LGPL-2.1 — voir `decision.md` ADR-006)
3.2 Détection BPM à l'import + beat-grid
3.3 Synchronisation automatique entre decks
**Dépend de** : Epic 2. Peut être menée en parallèle de l'Epic 4.

### Epic 4 — Plugins VST3 / Audio Unit
4.0 Trancher GUI native vs knobs génériques pour l'éditeur de plugin (voir `decision.md` ADR-011) — préalable à 4.3
4.1 Scan automatique des dossiers standards (VST3 + AU)
4.2 Sélection manuelle du chemin d'un plugin (bouton "Parcourir" / glisser-déposer)
4.3 Chaîne d'effets par deck (ajout / retrait / réordonnancement)
4.4 Isolation hors-process (au moins sur Windows) pour la stabilité
4.5 *(backlog, non priorisé)* évaluation du support VST2/FST selon les plugins réellement utilisés
**Dépend de** : Epic 2.

### Epic 5 — Bibliothèque de sons
5.1 Base SQLite (better-sqlite3), scan/import de dossiers
5.2 Métadonnées, tags, crates, cue points
**Dépend de** : Epic 2 (peut démarrer en parallèle des Epics 3/4).

### Epic 6 — Build local & lancement
Usage strictement personnel (voir `decision.md` ADR-010) : pas de notarization, pas de distribution.
6.1 electron-builder + build CMake combiné du module natif, packagé en `.app` local
6.2 Signature ad-hoc locale (`codesign --sign -`) pour éviter les blocages Gatekeeper au lancement
6.3 Script de build/lancement simple (one-liner) pour reconstruire après chaque évolution
**Dépend de** : toutes les epics fonctionnelles (dernier lot avant usage quotidien).

## Phase 3 — Implementation

Boucle recommandée par epic, avec Claude Code :
1. `bmad-sprint-planning` (une fois, pour initialiser `progress.md`/le suivi)
2. Pour chaque story : `bmad-create-story` → `bmad-dev-story` → `bmad-code-review`
3. Mise à jour de `progress.md` après chaque story
4. `bmad-retrospective` à la fin de chaque epic

Rappel des limites de l'automatisation (voir échange précédent) : la compilation réelle et l'écoute des résultats audio restent des étapes manuelles de Julien, en itération avec Claude Code.

## Ordre de séquence recommandé

```
Epic 1 (moteur standalone)
   └─► Epic 2 (intégration Electron)
          ├─► Epic 3 (pitch avancé)      ─┐
          ├─► Epic 4 (plugins VST3/AU)    ├─► Epic 6 (build local)
          └─► Epic 5 (bibliothèque)      ─┘
```

## Addon
Addons a voir quand tout sera fonctionnel, il s'agit de fonctionnalité a ajouter
- possibilité d'enregistrer le mix
- il faudrait avoir un affichage qui peut donner des infos (a voir commment changer les infos afficher mais au moins pouvoir montrer la répartition du fader)
- pouvoir mettre une loop sur un morceau, ou un certain nombre de secondes
- définir des raccourcis clavier.
- rendre le glisser de la platine (scratch/seek, Story 2.5) plus ergonomique — fonctionnel mais peu intuitif en l'état (sensibilité du drag, retour visuel pendant le geste).
