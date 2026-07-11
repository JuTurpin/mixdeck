// Colonne centrale "console master" (architecture.md §6, ~170px). Sans
// vumètres ni knobs Master/Booth/Cue Mix : pas de mesure de niveau ni de
// bus master/booth dans le moteur aujourd'hui.
import Crossfader from './Crossfader'
import type { MixerController } from '../controllers/MixerController'

interface ConsoleMasterProps {
  mixer: MixerController
}

export default function ConsoleMaster({ mixer }: ConsoleMasterProps) {
  return (
    <div
      style={{
        width: 170,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        background: '#16181d',
        borderRadius: 8,
        padding: 16
      }}
    >
      <Crossfader mixer={mixer} />
    </div>
  )
}
