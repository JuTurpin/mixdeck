// Barre de titre décorative (architecture.md §6) : pastilles façon feux
// macOS + titre de session. La fenêtre étant sans cadre natif (frame: false,
// voir main/index.ts), ces pastilles relaient les commandes de fenêtre
// équivalentes (rouge = fermer = quitter l'app, ADR-010 usage mono-fenêtre ;
// jaune = réduire ; vert = agrandir/restaurer).
import type { CSSProperties } from 'react'

const dots: { colour: string; onClick: () => void }[] = [
  { colour: '#f0645c', onClick: () => window.mixdeck.windowClose() },
  { colour: '#f5bd4f', onClick: () => window.mixdeck.windowMinimize() },
  { colour: '#61c454', onClick: () => window.mixdeck.windowToggleMaximize() }
]

export default function TitleBar() {
  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        gap: 12,
        padding: '10px 16px',
        background: '#101012',
        // La fenêtre est sans cadre natif (frame: false) : cette barre sert
        // aussi de zone de déplacement de la fenêtre.
        WebkitAppRegion: 'drag'
      } as CSSProperties}
    >
      <div style={{ display: 'flex', gap: 6 }}>
        {dots.map(({ colour, onClick }) => (
          <span
            key={colour}
            onClick={onClick}
            style={{
              width: 10,
              height: 10,
              borderRadius: '50%',
              background: colour,
              cursor: 'pointer',
              WebkitAppRegion: 'no-drag'
            } as CSSProperties}
          />
        ))}
      </div>
      <span style={{ fontSize: 12, color: '#8b8f98', letterSpacing: '0.04em' }}>
        MixDeck — Session
      </span>
    </div>
  )
}
