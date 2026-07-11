// Knob rotatif réutilisable (architecture.md §6 : "rotation ±50 mappée en
// angle"). Glisser verticalement change la valeur ; utilisé ici pour le
// filtre (-1..+1, centre = neutre), réutilisable plus tard pour d'autres
// paramètres du même type.
import { useCallback, useRef, type PointerEvent } from 'react'

interface FilterKnobProps {
  value: number // -1..+1
  onChange: (value: number) => void
  label?: string
}

const MIN_ANGLE = -135
const MAX_ANGLE = 135

export default function FilterKnob({ value, onChange, label }: FilterKnobProps) {
  const dragState = useRef<{ startY: number; startValue: number } | null>(null)

  const handlePointerDown = useCallback(
    (event: PointerEvent<HTMLDivElement>) => {
      dragState.current = { startY: event.clientY, startValue: value }
      event.currentTarget.setPointerCapture(event.pointerId)
    },
    [value]
  )

  const handlePointerMove = useCallback(
    (event: PointerEvent<HTMLDivElement>) => {
      if (!dragState.current) return
      const delta = dragState.current.startY - event.clientY // glisser vers le haut = augmente
      const newValue = Math.min(1, Math.max(-1, dragState.current.startValue + delta / 100))
      onChange(newValue)
    },
    [onChange]
  )

  const handlePointerUp = useCallback((event: PointerEvent<HTMLDivElement>) => {
    dragState.current = null
    event.currentTarget.releasePointerCapture(event.pointerId)
  }, [])

  const angle = MIN_ANGLE + ((value + 1) / 2) * (MAX_ANGLE - MIN_ANGLE)

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4 }}>
      <div
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        style={{
          width: 36,
          height: 36,
          borderRadius: '50%',
          background: '#1c1f26',
          border: '1px solid #2a2d35',
          position: 'relative',
          cursor: 'ns-resize',
          touchAction: 'none'
        }}
      >
        <div
          style={{
            position: 'absolute',
            top: 3,
            left: '50%',
            width: 2,
            height: 12,
            background: '#e8e9ec',
            transform: `translateX(-50%) rotate(${angle}deg)`,
            transformOrigin: '50% 15px'
          }}
        />
      </div>
      {label && <span style={{ fontSize: 10, color: '#8b8f98' }}>{label}</span>}
    </div>
  )
}
