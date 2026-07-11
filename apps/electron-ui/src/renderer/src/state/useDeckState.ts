import { useEffect, useState } from 'react'
import type { DeckController } from '../controllers'

export interface DeckStateSnapshot {
  state: string
  position: number
  length: number
}

// État rafraîchi par polling léger — solution explicitement temporaire
// (Story 2.4), remplacée par le modèle événementiel poussé par le moteur
// (ADR-015) en Story 2.6.
export function useDeckState(controller: DeckController, intervalMs = 300): DeckStateSnapshot {
  const [snapshot, setSnapshot] = useState<DeckStateSnapshot>({
    state: 'EMPTY',
    position: 0,
    length: 0
  })

  useEffect(() => {
    let cancelled = false

    const poll = async (): Promise<void> => {
      const [state, position, length] = await Promise.all([
        controller.getState(),
        controller.getPosition(),
        controller.getLength()
      ])
      if (!cancelled) setSnapshot({ state, position, length })
    }

    poll()
    const id = setInterval(poll, intervalMs)
    return () => {
      cancelled = true
      clearInterval(id)
    }
  }, [controller, intervalMs])

  return snapshot
}
