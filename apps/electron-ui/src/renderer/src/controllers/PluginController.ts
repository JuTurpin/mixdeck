import type { AvailablePlugin } from '../../../preload'

// Controller (ADR-014) : logique métier pour la découverte de plugins
// (Story 4.1 — scan VST3/AU, pas de chargement dans une chaîne d'effets
// encore, voir Story 4.3), entre React et le Bridge.
export class PluginController {
  scan(): Promise<void> {
    return window.mixdeck.startPluginScan()
  }

  isScanInProgress(): Promise<boolean> {
    return window.mixdeck.isPluginScanInProgress()
  }

  getAvailablePlugins(): Promise<AvailablePlugin[]> {
    return window.mixdeck.getAvailablePlugins()
  }

  // Story 4.2 (ADR-004) — sélection manuelle, VST3 uniquement. Retourne le
  // message d'erreur du Bridge ("" = succès), même convention que
  // DeckController.loadTrack().
  addPluginFromPath(path: string): Promise<string> {
    return window.mixdeck.addPluginFromPath(path)
  }

  // Ouvre le sélecteur de fichier natif puis charge le plugin choisi.
  // Retourne null si l'utilisateur annule, sinon le message d'erreur du
  // Bridge — même convention que DeckController.pickAndLoadTrack().
  async pickAndAddPlugin(): Promise<string | null> {
    const path = await window.mixdeck.pickPluginFile()
    if (!path) return null
    return this.addPluginFromPath(path)
  }
}
