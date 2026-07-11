// Crossfader horizontal (architecture.md §6). Le sélecteur de courbe n'est
// pas dans le mock mais reste une fonctionnalité réelle et déjà validée
// (Story 1.2) — gardé en discret à côté.
import type { MixerController } from '../controllers/MixerController'

interface CrossfaderProps {
  mixer: MixerController
}

export default function Crossfader({ mixer }: CrossfaderProps) {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 8 }}>
      <span style={{ fontSize: 10, color: '#8b8f98', letterSpacing: '0.06em' }}>CROSSFADER</span>
      <input
        type="range"
        min={0}
        max={1}
        step={0.01}
        defaultValue={0.5}
        onChange={(e) => mixer.setCrossfaderPosition(Number(e.target.value))}
        style={{ width: 140 }}
      />
      <select
        defaultValue="smooth"
        onChange={(e) => mixer.setCrossfaderCurve(e.target.value)}
        style={{
          fontSize: 11,
          background: '#1c1f26',
          color: '#e8e9ec',
          border: '1px solid #2a2d35',
          borderRadius: 4,
          padding: '2px 6px'
        }}
      >
        <option value="linear">Linéaire</option>
        <option value="smooth">Smooth</option>
        <option value="cut">Cut</option>
      </select>
    </div>
  )
}
