import { useEffect, useState } from 'react'
import type { DeckController } from '../controllers'

export interface DeckStateSnapshot {
  state: string
  position: number
  length: number
}

// Story 2.6 (ADR-015) — abonnement au flux d'événements poussé par le process
// principal (src/main/engineEvents.ts), filtré sur ce deck. Remplace le
// polling de la Story 2.4 : signature de retour inchangée, aucune adaptation
// nécessaire côté Deck.tsx.
export function useDeckState(controller: DeckController): DeckStateSnapshot {
  const [snapshot, setSnapshot] = useState<DeckStateSnapshot>({
    state: 'EMPTY',
    position: 0,
    length: 0
  })

  useEffect(() => {
    let cancelled = false

    // Valeur immédiate au montage, sans attendre le premier événement poussé.
    Promise.all([controller.getState(), controller.getPosition(), controller.getLength()]).then(
      ([state, position, length]) => {
        if (!cancelled) setSnapshot({ state, position, length })
      }
    )

    const unsubscribe = window.mixdeck.onEvent((event) => {
      if (event.deckIndex !== controller.deckIndex) return
      setSnapshot({ state: event.state, position: event.position, length: event.length })
    })

    return () => {
      cancelled = true
      unsubscribe()
    }
  }, [controller])

  return snapshot
}
