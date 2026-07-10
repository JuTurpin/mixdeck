const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  Header, Footer, AlignmentType, LevelFormat, ExternalHyperlink,
  TableOfContents, HeadingLevel, BorderStyle, WidthType, ShadingType,
  PageNumber, PageBreak,
} = require("docx");

const PAGE_WIDTH = 12240, PAGE_HEIGHT = 15840;
const MARGIN = 1440;
const CONTENT_WIDTH = PAGE_WIDTH - 2 * MARGIN; // 9360

const border = { style: BorderStyle.SINGLE, size: 2, color: "CCCCCC" };
const borders = { top: border, bottom: border, left: border, right: border };
const headerFill = "1F3864";
const altFill = "F2F4F7";

function p(text, opts = {}) {
  return new Paragraph({
    spacing: { after: 160, ...(opts.spacing || {}) },
    children: [new TextRun({ text, ...opts.run })],
    ...opts.pOpts,
  });
}

function h1(text) {
  return new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun(text)] });
}
function h2(text) {
  return new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun(text)] });
}
function bullet(text, ref = "bullets") {
  return new Paragraph({
    numbering: { reference: ref, level: 0 },
    spacing: { after: 100 },
    children: [new TextRun(text)],
  });
}

function cell(text, opts = {}) {
  return new TableCell({
    borders,
    width: { size: opts.width, type: WidthType.DXA },
    shading: opts.fill ? { fill: opts.fill, type: ShadingType.CLEAR } : undefined,
    margins: { top: 100, bottom: 100, left: 140, right: 140 },
    verticalAlign: "center",
    children: [new Paragraph({
      children: [new TextRun({ text, bold: !!opts.bold, color: opts.color || "1A1A1A" })],
    })],
  });
}

function table(widths, headerRow, rows) {
  return new Table({
    width: { size: widths.reduce((a, b) => a + b, 0), type: WidthType.DXA },
    columnWidths: widths,
    rows: [
      new TableRow({
        tableHeader: true,
        children: headerRow.map((t, i) => cell(t, { width: widths[i], fill: headerFill, bold: true, color: "FFFFFF" })),
      }),
      ...rows.map((r, ri) => new TableRow({
        children: r.map((t, i) => cell(t, { width: widths[i], fill: ri % 2 === 1 ? altFill : undefined })),
      })),
    ],
  });
}

function hr() {
  return new Paragraph({
    spacing: { before: 100, after: 300 },
    border: { bottom: { style: BorderStyle.SINGLE, size: 6, color: "1F3864", space: 1 } },
  });
}

const doc = new Document({
  styles: {
    default: { document: { run: { font: "Arial", size: 22, color: "1A1A1A" } } },
    paragraphStyles: [
      { id: "Title", name: "Title", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 48, bold: true, font: "Arial", color: "1F3864" },
        paragraph: { spacing: { before: 0, after: 120 } } },
      { id: "Subtitle", name: "Subtitle", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 26, italics: true, font: "Arial", color: "555555" },
        paragraph: { spacing: { after: 400 } } },
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 30, bold: true, font: "Arial", color: "1F3864" },
        paragraph: { spacing: { before: 420, after: 200 }, outlineLevel: 0,
          border: { bottom: { style: BorderStyle.SINGLE, size: 6, color: "1F3864", space: 4 } } } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 24, bold: true, font: "Arial", color: "2E5395" },
        paragraph: { spacing: { before: 280, after: 140 }, outlineLevel: 1 } },
    ],
  },
  numbering: {
    config: [
      { reference: "bullets", levels: [{ level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
        style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
      { reference: "numbers", levels: [{ level: 0, format: LevelFormat.DECIMAL, text: "%1.", alignment: AlignmentType.LEFT,
        style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
    ],
  },
  sections: [{
    properties: {
      page: { size: { width: PAGE_WIDTH, height: PAGE_HEIGHT }, margin: { top: MARGIN, right: MARGIN, bottom: MARGIN, left: MARGIN } },
    },
    headers: {
      default: new Header({ children: [new Paragraph({
        alignment: AlignmentType.RIGHT,
        children: [new TextRun({ text: "MixDeck — Document d'architecture", size: 16, color: "888888" })],
      })] }),
    },
    footers: {
      default: new Footer({ children: [new Paragraph({
        alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "Page ", size: 16, color: "888888" }), new TextRun({ children: [PageNumber.CURRENT], size: 16, color: "888888" })],
      })] }),
    },
    children: [
      new Paragraph({ style: "Title", children: [new TextRun("MixDeck")] }),
      new Paragraph({ style: "Subtitle", children: [new TextRun("Application de mixage DJ pour macOS — Document d'architecture")] }),
      p("Préparé pour Julien — 10 juillet 2026", { run: { color: "888888", size: 20 } }),
      new Paragraph({ children: [new PageBreak()] }),

      new TableOfContents("Sommaire", { hyperlink: true, headingStyleRange: "1-2" }),
      new Paragraph({ children: [new PageBreak()] }),

      // 1. Objectif
      h1("1. Objectif"),
      p("MixDeck est une application desktop (Electron, macOS) qui simule deux platines de mixage DJ. L'objectif est de pouvoir charger deux sons indépendamment sur deux “decks”, contrôler leur pitch/vitesse, les mixer via un crossfader et des faders individuels, appliquer un filtre par voie, et enrichir chaque deck d'une chaîne d'effets basée sur de vrais plugins VST3 / Audio Unit installés sur la machine — la même logique que Serato DJ Pro, Rekordbox ou Traktor."),
      p("Ce document couvre l'architecture technique cible, les choix structurants, et une feuille de route de développement. Il ne réutilise pas le code de l'application de bibliothèque existante : c'est un projet neuf, car le moteur audio temps réel nécessaire ici a des contraintes très différentes d'un gestionnaire de fichiers."),

      // 2. Décision clé
      h1("2. Décision d'architecture clé"),
      p("Le choix déterminant pour tout le reste du projet : héberger de vrais plugins VST3/AU. Ce n'est pas faisable de façon fiable en JavaScript / Web Audio API pur — ces formats sont des binaires natifs (C++) qui exposent une interface graphique et un protocole d'hôte spécifique. Cela impose un moteur audio natif écrit en C++, exposé à l'interface Electron via un module natif Node (N-API)."),
      p("Electron ne sert donc que de coquille d'interface (fenêtre, rendu des waveforms, contrôles). Tout le traitement audio temps réel — lecture, pitch, filtre, mixage, hébergement des plugins — est délégué au moteur natif. C'est le même schéma qu'un projet Electron+JUCE existant observé en recherche (bridge JUCE ↔ Electron via un module .node, avec hébergement des plugins hors-process sur Windows pour la stabilité)."),
      table(
        [2400, 3480, 3480],
        ["Critère", "Web Audio API seule (Electron/Chromium)", "Moteur natif JUCE + N-API"],
        [
          ["Plugins tiers VST3/AU", "Impossible nativement", "Supporté — approche standard de l'industrie"],
          ["Latence", "~20–40 ms selon buffer", "< 10 ms, réglable, niveau pro"],
          ["Pitch / time-stretch qualité pro", "Limité, à coder soi-même en JS, coûteux en CPU", "Bibliothèques matures (Rubber Band, SoundTouch)"],
          ["Complexité de développement", "Plus simple, 100% JS/TS", "C++ + bridge natif, build multi-cible"],
          ["Résistance à un plugin tiers instable", "Sans objet", "Nécessite un hébergement hors-process"],
        ]
      ),

      // 3. Vue d'ensemble
      h1("3. Vue d'ensemble des couches"),
      h2("3.1 Couche interface (Electron / React / TypeScript)"),
      p("Fenêtre principale, rendu des deux decks (waveform, jog wheel, boutons cue/loop), faders, crossfader, knob de filtre, browser de bibliothèque. Cette couche n'effectue aucun traitement audio : elle envoie des paramètres et reçoit des données d'affichage (position de lecture, niveau, données de waveform)."),
      h2("3.2 Couche pont (N-API)"),
      p("Module natif Node (node-addon-api) qui relie React/Electron au moteur C++. Transmet en continu les paramètres de contrôle (position, gain, pitch, cutoff du filtre, activation d'un plugin...) sans jamais bloquer le thread audio temps réel — c'est le point de vigilance principal pour éviter les glitches audio."),
      h2("3.3 Couche moteur audio (C++ / JUCE)"),
      p("Un AudioProcessor par deck (lecture fichier, pitch/time-stretch, filtre), un mixer (gains, courbe de crossfader), et un plugin host. Tourne sur son propre thread audio temps réel, indépendant du thread JavaScript."),
      h2("3.4 Couche hébergement des plugins"),
      p("Charge et fait tourner les plugins VST3/AU choisis par l'utilisateur dans la chaîne d'effets de chaque deck. Idéalement isolée en sous-processus par plugin (au moins sur Windows, selon les retours de projets similaires) pour qu'un plugin tiers instable ne fasse pas planter toute l'application ; sur macOS, l'hébergement Audio Unit se fait généralement dans le process principal du fait des contraintes AppKit."),

      // 4. Composants détaillés
      h1("4. Composants détaillés"),
      h2("4.1 Deux decks"),
      bullet("Lecture de fichiers audio via l'AudioFormatManager de JUCE (WAV, MP3, FLAC, AIFF...)."),
      bullet("Waveform précalculée à l'import et affichée côté UI (les données de “peaks” sont générées une fois puis mises en cache)."),
      bullet("Jog wheel virtuel pour le scratch et le pitch bend temporaire (nudge)."),
      bullet("Cue points et boucles (loop in/out) mémorisables."),
      h2("4.2 Pitch / tempo"),
      bullet("Slider de pitch classique (±8 / 16 / 50 %)."),
      bullet("Deux modes : “vitesse liée” (comme un vinyle — le pitch suit la vitesse, simple resampling) et “time-stretch” (le pitch reste indépendant de la vitesse), via une bibliothèque de time-stretch (Rubber Band ou SoundTouch — licence à vérifier selon le mode de distribution)."),
      bullet("Détection de BPM à l'import (analyse hors-ligne), avec possibilité de synchronisation automatique entre les deux decks."),
      h2("4.3 Fader et crossfader"),
      bullet("Fader de volume indépendant par deck (courbe linéaire ou logarithmique)."),
      bullet("Crossfader central avec courbes sélectionnables : linéaire, douce (“smooth”), ou “cut” façon scratch."),
      bullet("Le calcul de gain se fait entièrement côté moteur natif, jamais côté UI, pour éviter tout artefact audible."),
      h2("4.4 Filtre"),
      bullet("Un filtre résonant par deck, multimode (passe-bas / passe-haut), commandé par un knob unique : centre = neutre, à gauche = passe-bas de plus en plus fermé, à droite = passe-haut de plus en plus ouvert — comportement standard des mixers DJ."),
      bullet("Implémentation DSP native (ex. filtre state-variable de JUCE), sans latence perceptible."),
      h2("4.5 Chaîne d'effets — plugins VST3 / Audio Unit"),
      p("Chaque deck (et éventuellement le bus master) dispose d'un emplacement de chaîne d'effets pouvant charger un ou plusieurs plugins VST3 (Windows/macOS) et Audio Unit (macOS uniquement)."),
      p("Point important à clarifier sur le terme “téléchargeable” : pour des raisons de licence et de propriété intellectuelle, MixDeck ne peut pas distribuer ou télécharger automatiquement des plugins commerciaux tiers depuis l'application elle-même. L'approche standard, utilisée par Ableton Live, Serato DJ Pro ou Rekordbox, est la suivante :"),
      bullet("MixDeck scanne les emplacements standards des plugins déjà installés par l'utilisateur (ex. ~/Library/Audio/Plug-Ins/VST3 et /Components sur macOS)."),
      bullet("L'utilisateur installe ses plugins via les installateurs officiels des éditeurs (y compris des plugins gratuits reconnus : TAL, Valhalla Freeverb, Airwindows...)."),
      bullet("MixDeck propose une interface pour activer/organiser ces plugins dans la chaîne d'effets de chaque deck, avec des liens de référence vers des plugins gratuits recommandés."),
      bullet("En complément du scan automatique, l'utilisateur peut indiquer manuellement le chemin d'un fichier de plugin (bouton “Parcourir” ou glisser-déposer). Utile pour les plugins installés hors des dossiers standards, ou pour tester un plugin ponctuel sans le déplacer."),

      h2("4.6 Cas particulier : plugins au format VST2 / FST"),
      p("FST est une réimplémentation open source (licence GPL) des en-têtes du SDK VST2 de Steinberg, créée après l'arrêt de la distribution officielle de ce SDK par Steinberg en 2018. Un plugin “FST” est donc, en pratique, un plugin VST2 classique — même binaire, même protocole d'hôte — simplement compilé avec ces en-têtes libres plutôt qu'avec le SDK propriétaire de Steinberg."),
      p("Sur macOS, un plugin VST2 se présente normalement en bundle .vst, pas avec une extension .fst : si tes fichiers portent bien l'extension .fst, il s'agit peut-être d'un format lié à un pipeline différent (le projet FST propose aussi un pont pour héberger des VST Windows sous Linux via Wine) — à vérifier au cas par cas selon l'origine de ces plugins."),
      p("Implication pour l'architecture : JUCE a retiré son support VST2 intégré en 2018 suite au retrait du SDK par Steinberg. Pour héberger du VST2/FST aujourd'hui, il faut soit récupérer l'ancien SDK VST2 (statut légal flou, Steinberg ne le distribue plus officiellement), soit s'appuyer sur les en-têtes FST eux-mêmes — mais ceux-ci sont sous licence GPL, ce qui impose des contraintes de licence sur tout moteur qui les intégrerait directement."),
      p("Bonne nouvelle : le SDK VST3 est passé open source sous licence MIT en octobre 2025, ce qui simplifie nettement son intégration. Recommandation : prioriser VST3 et Audio Unit comme formats supportés en V1 (aucun problème de licence), et traiter le support VST2/FST comme une option secondaire à évaluer plus tard selon les plugins que tu utilises réellement."),

      h2("4.7 Bibliothèque de sons"),
      bullet("Métadonnées (titre, artiste, BPM, tonalité, cue points) stockées en base locale SQLite (better-sqlite3)."),
      bullet("Scan/import de dossiers, gestion de tags et de crates, cohérent avec les habitudes de l'app de bibliothèque existante."),

      // 5. Stack technique
      h1("5. Stack technique recommandée"),
      table(
        [2600, 6760],
        ["Composant", "Choix technique"],
        [
          ["Interface", "Electron + React + TypeScript. Web Audio API utilisée uniquement pour l'affichage (waveform), jamais pour le traitement du signal."],
          ["Moteur audio", "C++ / JUCE — un AudioProcessor par deck, mixer, DSP (filtre, time-stretch), plugin host."],
          ["Pont natif", "node-addon-api (N-API) — pattern éprouvé sur des projets Electron + JUCE existants."],
          ["Hébergement plugins", "JUCE AudioPluginFormatManager (VST3 + AU), hébergement hors-process recommandé pour la stabilité."],
          ["Time-stretch", "Rubber Band Library ou SoundTouch (vérifier la licence selon le mode de distribution retenu)."],
          ["Base de données", "SQLite via better-sqlite3."],
          ["Build / packaging", "electron-builder pour l'app, CMake pour le module natif, notarization Apple obligatoire."],
        ]
      ),

      // 6. Défis
      h1("6. Défis techniques et risques"),
      bullet("Build plus complexe : compilation croisée d'un module natif C++/JUCE avec Electron (CMake, prebuild par version d'ABI Node/Electron)."),
      bullet("Stabilité : un plugin VST/AU tiers buggé peut faire planter tout le processus — nécessite une isolation (sous-processus), en particulier pour des plugins non testés."),
      bullet("Distribution macOS : la notarization Apple et le sandboxing sont plus contraignants avec un hébergement de plugins tiers hors sandbox — une distribution hors Mac App Store (comme Ableton ou Serato) est probablement nécessaire."),
      bullet("Le thread audio C++ doit rester totalement indépendant du thread JavaScript (mono-thread) d'Electron pour garantir un comportement temps réel."),
      bullet("Le time-stretch de qualité professionnelle reste un sujet technique pointu : mal réglé, il produit des artefacts audibles."),

      // 7. Roadmap
      h1("7. Feuille de route proposée"),
      table(
        [1400, 2200, 5760],
        ["Phase", "Nom", "Contenu"],
        [
          ["1", "Fondations", "Moteur JUCE autonome (hors Electron) : lecture de deux pistes, gains, crossfader, filtre. Valider le DSP en standalone avant toute intégration."],
          ["2", "Intégration Electron", "Bridge N-API, interface React avec deux decks, waveform, contrôles de base connectés au moteur."],
          ["3", "Pitch avancé", "Time-stretch indépendant, détection BPM / beat-grid, synchronisation automatique entre decks."],
          ["4", "Plugins", "Scan et hébergement VST3/AU, interface de gestion de la chaîne d'effets, isolation hors-process."],
          ["5", "Bibliothèque & finition", "Intégration de la bibliothèque de sons, cue points, boucles, packaging et signature macOS."],
        ]
      ),

      // 8. Alternative
      h1("8. Option de repli"),
      p("Si le coût d'ingénierie du moteur natif s'avère trop élevé dans un premier temps, une version 1 entièrement en Web Audio API reste possible — sans VST/AU réels, avec des effets “façon plugin” codés en JS (reverb, delay, filtre, bitcrush). Cette option n'a pas été retenue comme cible principale puisque le support de vrais plugins VST/AU a été choisi comme objectif, mais elle reste un chemin utile pour un prototype rapide, quitte à migrer ensuite vers le moteur JUCE."),

      // 9. Prochaines étapes
      h1("9. Prochaines étapes"),
      p("Recommandation : commencer par un POC du moteur JUCE seul (Phase 1) validant lecture, crossfader, filtre et pitch en standalone, avant d'investir dans le bridge Electron. Cela permet de vérifier rapidement la faisabilité et la latence réelle avant d'engager le développement de l'interface."),

      hr(),
      h2("Références"),
      new Paragraph({ children: [
        new ExternalHyperlink({ children: [new TextRun({ text: "JUCE Framework — hébergement de plugins audio", style: "Hyperlink" })], link: "https://juce.com" }),
      ] }),
      new Paragraph({ children: [
        new ExternalHyperlink({ children: [new TextRun({ text: "Exemple d'architecture Electron + JUCE + N-API (slopsmith-desktop)", style: "Hyperlink" })], link: "https://deepwiki.com/byrongamatos/slopsmith-desktop" }),
      ] }),
      new Paragraph({ children: [
        new ExternalHyperlink({ children: [new TextRun({ text: "vst-js — addon natif Node pour VST3", style: "Hyperlink" })], link: "https://github.com/ramirezd42/vst-js" }),
      ] }),
    ],
  }],
});

Packer.toBuffer(doc).then(buffer => {
  fs.writeFileSync("/sessions/compassionate-gracious-fermat/mnt/outputs/MixDeck_Architecture.docx", buffer);
  console.log("done");
});
