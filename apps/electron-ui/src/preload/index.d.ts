import type { MixdeckBridgeApi } from './index'

declare global {
  interface Window {
    mixdeck: MixdeckBridgeApi
  }
}
