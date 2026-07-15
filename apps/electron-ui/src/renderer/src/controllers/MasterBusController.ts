import type { PluginChainSlot } from '../../../preload'

// Controller (ADR-014) : logique métier pour la chaîne d'effets du bus
// master (Story 4.3) — même forme que DeckController's plugin-chain methods,
// sans index de deck (un seul bus master).
export class MasterBusController {
  addPlugin(identifier: string): Promise<void> {
    return window.mixdeck.masterAddPlugin(identifier)
  }

  isAddingPlugin(): Promise<boolean> {
    return window.mixdeck.masterIsAddingPlugin()
  }

  getLastPluginAddError(): Promise<string> {
    return window.mixdeck.masterGetLastPluginAddError()
  }

  removePlugin(index: number): Promise<void> {
    return window.mixdeck.masterRemovePlugin(index)
  }

  movePlugin(fromIndex: number, toIndex: number): Promise<void> {
    return window.mixdeck.masterMovePlugin(fromIndex, toIndex)
  }

  setPluginBypassed(index: number, bypassed: boolean): Promise<void> {
    return window.mixdeck.masterSetPluginBypassed(index, bypassed)
  }

  showPluginEditor(index: number): Promise<void> {
    return window.mixdeck.masterShowPluginEditor(index)
  }

  hidePluginEditor(index: number): Promise<void> {
    return window.mixdeck.masterHidePluginEditor(index)
  }

  getPluginChain(): Promise<PluginChainSlot[]> {
    return window.mixdeck.masterGetPluginChain()
  }
}
