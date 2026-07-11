import type { WebContents } from 'electron'
import { MIXDECK_EVENT_CHANNEL, type MixdeckEvent, type MixdeckEventType } from '../shared/events'

// Story 2.6 (ADR-015) — remplace le polling du renderer (Story 2.4) : le
// process principal lit l'état directement sur nativeEngine (appel natif,
// pas d'IPC puisqu'on est déjà dans ce process) et POUSSE les changements au
// renderer via webContents.send, plutôt que de laisser le renderer demander.
const POLL_INTERVAL_MS = 200

// Le moteur ne distingue pas un arrêt utilisateur d'une fin de piste
// naturelle (les deux donnent DeckState::Stopped, voir Story 2.1) : on le
// déduit ici en comparant la position à la durée au moment de la transition.
const TRACK_ENDED_EPSILON_SECONDS = 0.5

interface DeckSnapshot {
  state: string
  position: number
  length: number
}

function eventTypeForTransition(
  previousState: string,
  snapshot: DeckSnapshot
): MixdeckEventType | null {
  if (previousState === snapshot.state) return null

  switch (snapshot.state) {
    case 'READY':
      return 'TrackLoaded'
    case 'PLAYING':
      return 'PlaybackStarted'
    case 'PAUSED':
      return 'PlaybackPaused'
    case 'STOPPED':
      return snapshot.length - snapshot.position < TRACK_ENDED_EPSILON_SECONDS
        ? 'TrackEnded'
        : 'PlaybackStopped'
    case 'ERROR':
      return 'EngineError'
    default:
      return null
  }
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export function startEngineEventPump(nativeEngine: any, webContents: WebContents): () => void {
  const lastState = ['EMPTY', 'EMPTY']

  const id = setInterval(() => {
    for (let deckIndex = 0; deckIndex < 2; deckIndex++) {
      const snapshot: DeckSnapshot = {
        state: nativeEngine.deckGetState(deckIndex),
        position: nativeEngine.deckGetPosition(deckIndex),
        length: nativeEngine.deckGetLength(deckIndex)
      }

      const transitionType = eventTypeForTransition(lastState[deckIndex], snapshot)
      lastState[deckIndex] = snapshot.state

      if (transitionType) {
        const event: MixdeckEvent = { type: transitionType, deckIndex, ...snapshot }
        console.log('[MixDeck event]', event.type, 'deck', deckIndex)
        webContents.send(MIXDECK_EVENT_CHANNEL, event)
      }

      const positionEvent: MixdeckEvent = { type: 'PositionChanged', deckIndex, ...snapshot }
      webContents.send(MIXDECK_EVENT_CHANNEL, positionEvent)
    }
  }, POLL_INTERVAL_MS)

  return () => clearInterval(id)
}
