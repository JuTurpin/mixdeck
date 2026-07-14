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
}
