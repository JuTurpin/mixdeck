// Story 2.6 (ADR-015) — modèle événementiel moteur → UI, partagé entre main
// (émetteur, src/main/engineEvents.ts), preload (relai) et renderer
// (consommateur, src/renderer/src/state/useDeckState.ts).
// Pas de PeakMeterChanged : différé, aucune mesure de niveau côté moteur
// (ADR-013, domaine Monitoring).
export const MIXDECK_EVENT_CHANNEL = 'mixdeck:event'

export type MixdeckEventType =
  | 'TrackLoaded'
  | 'PlaybackStarted'
  | 'PlaybackPaused'
  | 'PlaybackStopped'
  | 'TrackEnded'
  | 'EngineError'
  | 'PositionChanged'

export interface MixdeckEvent {
  type: MixdeckEventType
  deckIndex: number
  state: string
  position: number
  length: number
}
