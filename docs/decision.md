# MixDeck — decision.md

> Journal des décisions techniques (ADR — Architecture Decision Record).
> Dernière mise à jour : 2026-07-10
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
**Statut** : Accepté (à confirmer en implémentation, Epic 4.4).

### ADR-006 — Bibliothèque de time-stretch : Rubber Band ou SoundTouch
**Contexte** : le pitch indépendant de la vitesse nécessite un algorithme de time-stretch de qualité.
**Décision** : évaluer Rubber Band Library et SoundTouch (LGPL) ; le choix final dépendra de la licence compatible avec le mode de distribution retenu.
**Statut** : Ouvert — décision finale à prendre en Epic 3.

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

### ADR-011 — Interface d'édition des plugins : GUI native vs knobs génériques (ouvert)
**Contexte** : le mock de design (Claude Design) montre une modale avec 6 knobs génériques ("Param 1"...) pour éditer un plugin, plutôt que l'interface graphique propre au plugin.
**Décision à prendre** : deux options possibles à trancher en Epic 4 —
  1. **GUI native du plugin** (fenêtre native JUCE embarquant l'éditeur réel du plugin) : fidèle à l'expérience DAW habituelle, mais plus complexe à intégrer dans une fenêtre Electron/React (nécessite une vue native superposée).
  2. **Surface de contrôle générique** (knobs mappés sur les premiers paramètres exposés par le plugin, comme dans le mock) : plus simple à coder en React et cohérent visuellement avec le reste de l'app, mais incomplet pour des plugins à interface complexe.
**Statut** : Ouvert — à trancher avant de démarrer la story 4.3 (chaîne d'effets par deck).

### ADR-012 — Politique de sécurité des dépendances : épinglage strict + SBOM + veille vulnérabilités
**Contexte** : dès la première dépendance externe du projet (submodule JUCE, Story 1.1), Julien souhaite poser une philosophie durable de gestion des mises à jour de sécurité, plutôt que de l'improviser epic après epic.
**Décision** :
- Toute dépendance externe (submodule C++, bibliothèque vendorisée, package npm) est **épinglée à une version/commit exact** — jamais de plage de version, jamais `latest`/`main`/`HEAD`.
- Un **SBOM** (`docs/sbom.json`, format CycloneDX) est tenu à jour à chaque ajout ou mise à jour de dépendance : nom, version, commit/hash, licence, URL du dépôt.
- La veille de vulnérabilités est effectuée avant toute montée de version d'une dépendance : consultation des security advisories du projet concerné (ex. [GitHub Security Advisories de JUCE](https://github.com/juce-framework/JUCE/security)) côté C++/submodules ; `npm audit` + GitHub Dependabot côté npm (à partir de l'Epic 2, quand Electron/node-addon-api arrivent).
- En cas de vulnérabilité détectée, la mise à jour est **ciblée sur la seule dépendance concernée** — jamais de mise à jour groupée ou de refactor opportuniste des autres dépendances à cette occasion.
**Alternatives écartées** : dépendances non épinglées (branches/tags mouvants) — impossible à auditer de façon fiable, casse la reproductibilité des builds.
**Statut** : Accepté.
