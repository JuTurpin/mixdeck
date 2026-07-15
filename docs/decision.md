# MixDeck — decision.md

> Journal des décisions techniques (ADR — Architecture Decision Record).
> Dernière mise à jour : 2026-07-15
> Voir aussi : `architecture.md`, `roadmap.md`, `progress.md`

Chaque entrée : contexte → décision → alternatives écartées → statut.

---

### ADR-001 — Moteur audio natif (C++/JUCE) plutôt que Web Audio API pur
**Contexte** : besoin d'héberger de vrais plugins VST3/Audio Unit tiers.
**Décision** : le traitement audio temps réel (lecture, pitch, filtre, mixage, plugin host) est délégué à un moteur natif C++/JUCE. Electron reste une coquille d'interface.
**Alternatives écartées** : Web Audio API pur — ne permet pas d'héberger des plugins binaires VST3/AU, latence moins bonne (~20-40 ms vs <10 ms).
**Statut** : Accepté.

### ADR-002 — Bridge Electron ↔ moteur natif via N-API
**Contexte** : il faut relier l'UI React/Electron au moteur C++ sans bloquer le thread audio.
**Décision** : module natif Node construit avec `node-addon-api`, pattern déjà observé sur des projets Electron+JUCE existants (ex. slopsmith-desktop).
**Statut** : Accepté.

### ADR-003 — Formats de plugins supportés en V1 : VST3 + Audio Unit uniquement
**Contexte** : Julien possède des plugins VST2 au format FST (réimplémentation open source, GPL, des en-têtes VST2 de Steinberg — le SDK VST2 officiel a été retiré par Steinberg en 2018, JUCE a retiré son support VST2 intégré à la même période).
**Décision** : prioriser VST3 et Audio Unit en V1. Le SDK VST3 est passé open source sous licence MIT en octobre 2025, ce qui lève toute ambiguïté de licence. Le support VST2/FST est repoussé en backlog (Epic 4.5), à évaluer selon les plugins réellement utilisés — son intégration impliquerait soit l'ancien SDK VST2 (statut légal flou), soit les en-têtes FST (GPL, avec implications sur la licence du moteur).
**Alternatives écartées** : support VST2/FST dès la V1 — complexité et incertitude légale non justifiées pour un premier lot.
**Statut** : Accepté.

### ADR-004 — Chargement des plugins : scan automatique + sélection manuelle du chemin
**Contexte** : Julien veut pouvoir spécifier lui-même le chemin d'un plugin, pas seulement dépendre d'un scan automatique.
**Décision** : en plus du scan des dossiers standards (`~/Library/Audio/Plug-Ins/VST3`, `/Components`), l'UI propose un bouton "Parcourir" / glisser-déposer pour charger un plugin depuis n'importe quel chemin.
**Statut** : Accepté.

### ADR-005 — Hébergement des plugins hors-process (au moins Windows)
**Contexte** : un plugin tiers instable ne doit pas faire planter toute l'application.
**Décision** : isolation hors-process recommandée, en particulier sur Windows. Sur macOS, l'hébergement Audio Unit reste généralement dans le process principal du fait des contraintes AppKit.
**Confirmé en implémentation (Story 4.4.1)** : MixDeck ne ciblant que macOS, seul VST3 est isolé (process dédié `MixDeckPluginWorker` par instance, `juce::ChildProcessCoordinator`/`Worker`) ; AU reste en process principal comme anticipé. Compromis assumé avec Julien : l'éditeur d'un plugin VST3 isolé s'ouvre en fenêtre indépendante réelle plutôt qu'incrusté (incrustation cross-process non réalisable sur macOS sans API privées fragiles). Le streaming audio réel à travers un plugin isolé est repoussé à la Story 4.4.2 (4.4.1 couvre le cycle de vie/la détection de crash, pas encore le traitement du signal).
**Statut** : Accepté.

### ADR-006 — Bibliothèque de time-stretch : SoundTouch
**Contexte** : le pitch indépendant de la vitesse (Epic 3, Story 3.1) nécessite un algorithme de time-stretch de qualité, en plus du mode "vitesse liée" existant (resampling, Story 1.4). Deux candidates évaluées : Rubber Band Library (GPLv2+, licence commerciale requise seulement en cas de distribution propriétaire) et SoundTouch (LGPL-2.1). Usage strictement personnel non distribué (ADR-010) : aucune des deux licences n'est bloquante.
**Décision** : SoundTouch (LGPL-2.1), choix de Julien.
**Alternatives écartées** : Rubber Band Library — qualité généralement jugée supérieure sur des ratios de stretch importants, mais non retenue.
**Statut** : Accepté.

### ADR-007 — Bibliothèque de sons : SQLite local (better-sqlite3)
**Contexte** : besoin de stocker métadonnées, tags, crates, cue points sans dépendance serveur.
**Décision** : SQLite via `better-sqlite3`, cohérent avec l'approche de l'app de bibliothèque existante (le code n'est pas réutilisé, mais la logique de stockage locale l'est).
**Statut** : Accepté.

### ADR-008 — Processus de développement : BMAD + Claude Code, avec étapes manuelles identifiées
**Contexte** : Julien souhaite s'appuyer sur Claude Code pour l'intégralité du développement.
**Décision** : développement structuré selon les 4 phases BMAD (Analysis → Planning → Solutioning → Implementation), documents vivants dans `docs/` (`architecture.md`, `roadmap.md`, `decision.md`, `progress.md`). Claude Code écrit le code (UI, moteur C++/JUCE, bindings). Restent manuelles : compilation/tests sur machine réelle, évaluation à l'oreille de la qualité audio.
**Statut** : Accepté.

### ADR-009 — Projet neuf, sans réutilisation du code de l'app de bibliothèque existante
**Contexte** : une app de gestion de bibliothèque de sample existe déjà en Electron.
**Décision** : MixDeck part d'une base de code neuve — les contraintes temps réel du moteur audio sont trop différentes pour justifier une réutilisation directe.
**Statut** : Accepté (décision initiale de Julien).

### ADR-010 — Pas de distribution : usage strictement personnel, build local
**Contexte** : Julien sera le seul utilisateur de l'application, pas d'intention de la partager ni de la publier.
**Décision** : on retire tout ce qui relevait de la distribution publique — notarization Apple, Developer ID, Mac App Store. L'app est buildée et lancée localement sur sa machine. Une signature ad-hoc (`codesign --sign -`, gratuite, locale) suffit pour que Gatekeeper laisse tourner le binaire construit sur place ; si Gatekeeper bloque malgré tout un premier lancement, un simple clic droit → "Ouvrir" (ou `xattr -cr` sur le `.app`) suffit, sans compte développeur Apple ni frais annuels (99 $/an autrement).
**Alternatives écartées** : notarization + Developer ID (ADR-008 initial) — inutile et coûteux (abonnement Apple Developer) pour un usage mono-poste.
**Impact** : Epic 6 de `roadmap.md` simplifié (renommé "Build local & lancement"), suppression des stories liées à la signature/notarization/distribution.
**Statut** : Accepté.

### ADR-011 — Interface d'édition des plugins : GUI native du plugin
**Contexte** : le mock de design (Claude Design) montre une modale avec 6 knobs génériques ("Param 1"...) pour éditer un plugin, plutôt que l'interface graphique propre au plugin.
**Décision** : GUI native du plugin (fenêtre native JUCE embarquant l'éditeur réel du plugin) — fidèle à l'expérience DAW habituelle, choix de Julien. Implique de lier la variante GUI de `juce_audio_processors` côté Bridge Electron (aujourd'hui seule la variante headless, sans GUI, est liée dans `engine-core`, partagée avec le Bridge Node qui n'a pas de fenêtre), et de résoudre l'incrustation d'une fenêtre/vue native JUCE par-dessus ou dans la fenêtre Electron — un chantier de windowing natif à part entière, à cadrer avant/pendant la story 4.3.
**Alternatives écartées** : surface de contrôle générique (knobs mappés sur les premiers paramètres exposés par le plugin, comme dans le mock) — restait entièrement dans l'architecture headless actuelle, plus simple à intégrer en React, mais incomplète pour des plugins à interface riche ; non retenue.
**Statut** : Accepté.

### ADR-012 — Politique de sécurité des dépendances : épinglage strict + SBOM + veille vulnérabilités
**Contexte** : dès la première dépendance externe du projet (submodule JUCE, Story 1.1), Julien souhaite poser une philosophie durable de gestion des mises à jour de sécurité, plutôt que de l'improviser epic après epic.
**Décision** :
- Toute dépendance externe (submodule C++, bibliothèque vendorisée, package npm) est **épinglée à une version/commit exact** — jamais de plage de version, jamais `latest`/`main`/`HEAD`.
- Un **SBOM** (`docs/sbom.json`, format CycloneDX) est tenu à jour à chaque ajout ou mise à jour de dépendance : nom, version, commit/hash, licence, URL du dépôt.
- La veille de vulnérabilités est effectuée avant toute montée de version d'une dépendance : consultation des security advisories du projet concerné (ex. [GitHub Security Advisories de JUCE](https://github.com/juce-framework/JUCE/security)) côté C++/submodules ; `npm audit` + GitHub Dependabot côté npm (à partir de l'Epic 2, quand Electron/node-addon-api arrivent).
- En cas de vulnérabilité détectée, la mise à jour est **ciblée sur la seule dépendance concernée** — jamais de mise à jour groupée ou de refactor opportuniste des autres dépendances à cette occasion.
**Alternatives écartées** : dépendances non épinglées (branches/tags mouvants) — impossible à auditer de façon fiable, casse la reproductibilité des builds.
**Statut** : Accepté.

### ADR-013 — Contrat "Engine API" défini avant l'UI Electron
**Contexte** : avant de développer l'UI React de l'Epic 2, revue d'architecture (`docs/update.md`, Julien) demandant de poser un contrat clair entre Electron et JUCE plutôt que de laisser React dépendre directement de l'implémentation du moteur.
**Décision** : définir une surface d'API minimale avant tout code Bridge/UI (Story 2.1) — `Deck` (loadTrack/unloadTrack/play/pause/stop/seek), `Mixer` (setGain/setCrossfader/setFilter), `Plugins` (addPlugin/removePlugin/movePlugin/setPluginParameter), `Monitoring` (getPosition/getPeakMeter/getWaveform). Cette API cible sera mappée en Story 2.1 sur les méthodes C++ déjà validées de `Deck`/`Mixer` (Epic 1 : `play()`, `stop()`, `setGain()`, `setFilterKnob()`, `setPitch()`) — sans modifier ce code Epic 1.
**Alternatives écartées** : laisser React appeler directement les méthodes du module natif au fil de l'eau — couplage fort UI/moteur, refactors du moteur qui cassent l'UI.
**Statut** : Accepté.

### ADR-014 — Couche Controller entre React et le Bridge
**Contexte** : l'architecture initiale esquissait `React → Bridge → JUCE` (voir `architecture.md` §3 avant révision). Revue d'architecture avant Epic 2 : le Bridge ne doit contenir aucune logique métier, seulement de la traduction JS ↔ C++.
**Décision** : insérer une couche Controller — `React → Controller → Bridge → JUCE`. Le Controller (JS/TS, côté `apps/electron-ui`) porte la logique métier ; le Bridge (N-API/`NodeBinding.cpp`) reste un simple traducteur d'appels et de types.
**Alternatives écartées** : Bridge "gros" avec logique métier intégrée — mélange les responsabilités, rend le module natif plus difficile à faire évoluer indépendamment de l'UI.
**Statut** : Accepté.

### ADR-015 — Communication moteur → UI événementielle
**Contexte** : au-delà des appels UI → moteur (commandes), l'UI a besoin d'être notifiée des changements côté moteur (position de lecture, fin de piste, erreurs...).
**Décision** : flux `JUCE → Bridge → EventBus/Store → React`. Événements prévus : `TrackLoaded`, `PlaybackStarted`, `PlaybackPaused`, `PlaybackStopped`, `PositionChanged`, `PeakMeterChanged`, `TrackEnded`, `EngineError`. Les événements haute fréquence (position, vumètres) sont agrégés/throttlés avant d'atteindre React, pour ne pas saturer l'UI.
**Alternatives écartées** : polling depuis React (React interroge périodiquement l'état du moteur) — plus simple mais moins réactif et introduit des appels synchrones répétés UI → moteur, à l'opposé du principe "limiter les appels synchrones" (voir bonnes pratiques temps réel, `architecture.md` §5).
**Statut** : Accepté.

### ADR-016 — Machine d'état des Decks comme source de vérité UI
**Contexte** : sans modèle d'état explicite, l'UI risque de reconstruire son propre état à partir de multiples booléens (`isPlaying`, `isLoading`...), source de désynchronisations avec le moteur.
**Décision** : chaque Deck expose un état parmi `EMPTY / LOADING / READY / PLAYING / PAUSED / STOPPED / ERROR`. L'UI se base uniquement sur cet état (remonté via les événements de l'ADR-015), jamais sur une reconstruction ad hoc côté React.
**Alternatives écartées** : état implicite déduit de plusieurs flags côté UI — fragile, désynchronisation facile avec l'état réel du moteur.
**Statut** : Accepté.
