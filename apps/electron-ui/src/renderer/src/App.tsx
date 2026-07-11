// Story 2.3 — écran minimal (initialisation Electron uniquement). Les vrais
// composants Deck/Crossfader/Filtre arrivent en Story 2.5+ (voir architecture.md §6).
export default function App(): JSX.Element {
  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        height: '100vh',
        background: '#0d0e11',
        color: '#e8e9ec',
        fontFamily: 'system-ui, -apple-system, sans-serif'
      }}
    >
      <h1 style={{ fontWeight: 600, letterSpacing: '0.02em' }}>MixDeck</h1>
    </div>
  )
}
