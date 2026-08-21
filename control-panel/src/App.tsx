import {useEffect, useRef, useState, type KeyboardEvent} from 'react'
import {
  isWebBluetoothSupported,
  TennisGunBleClient,
  type ControlCommand,
  type DeviceState,
} from './ble.ts'

type ConnectionState = 'disconnected' | 'connecting' | 'connected'

type SpeedControlProps = {
  disabled: boolean
  label: string
  value: number
  onChange: (value: number) => void
  onCommit: (value: number) => void
}

type ErrorToastProps = {
  message: string
  onClose: () => void
}

const COMMIT_KEYS = new Set([
  'ArrowDown', 'ArrowLeft', 'ArrowRight', 'ArrowUp',
  'End', 'Home', 'PageDown', 'PageUp',
])

function SpeedControl({disabled, label, value, onChange, onCommit}: SpeedControlProps) {
  const handleKeyDown = (event: KeyboardEvent<HTMLInputElement>) => {
    if (!COMMIT_KEYS.has(event.key)) return
    event.preventDefault()

    const currentValue = event.currentTarget.valueAsNumber
    const nextValue = (() => {
      switch (event.key) {
        case 'Home': return 0
        case 'End': return 100
        case 'PageDown': return Math.max(0, currentValue - 10)
        case 'PageUp': return Math.min(100, currentValue + 10)
        case 'ArrowDown':
        case 'ArrowLeft': return Math.max(0, currentValue - 1)
        default: return Math.min(100, currentValue + 1)
      }
    })()

    event.currentTarget.value = String(nextValue)
    onChange(nextValue)
  }

  return (
    <section className="flex h-full flex-col items-center">
      <label className="mb-4 text-center text-xl font-semibold" htmlFor={`speed-${label}`}>
        {label} <span className="inline-block min-w-16">{value}%</span>
      </label>
      <input
        aria-valuetext={`${value}%`}
        className="range range-primary range-xl range-vertical h-full"
        disabled={disabled}
        id={`speed-${label}`}
        max={100}
        min={0}
        onInput={event => onChange(event.currentTarget.valueAsNumber)}
        onKeyDown={handleKeyDown}
        onKeyUp={event => {
          if (COMMIT_KEYS.has(event.key)) onCommit(event.currentTarget.valueAsNumber)
        }}
        onPointerCancel={event => onCommit(event.currentTarget.valueAsNumber)}
        onPointerUp={event => onCommit(event.currentTarget.valueAsNumber)}
        type="range"
        value={value}
      />
    </section>
  )
}

function ErrorToast({message, onClose}: ErrorToastProps) {
  return (
    <div className="toast toast-center toast-top z-50" role="alert" aria-live="assertive">
      <div className="alert alert-error gap-4 shadow-lg">
        <span>{message}</span>
        <button className="btn btn-ghost btn-sm" onClick={onClose} type="button" aria-label="Dismiss error">
          Close
        </button>
      </div>
    </div>
  )
}

function App() {
  const supported = isWebBluetoothSupported()
  const clientRef = useRef<TennisGunBleClient | undefined>(undefined)
  const pendingCountRef = useRef(0)
  const [connection, setConnection] = useState<ConnectionState>('disconnected')
  const [state, setState] = useState<DeviceState>()
  const [error, setError] = useState<string>()
  const [pendingCommand, setPendingCommand] = useState(false)
  const [topSpeed, setTopSpeed] = useState(0)
  const [bottomSpeed, setBottomSpeed] = useState(0)
  const [deliverySpeed, setDeliverySpeed] = useState(0)

  useEffect(() => {
    if (!error) return
    const timeout = window.setTimeout(() => setError(undefined), 5000)
    return () => window.clearTimeout(timeout)
  }, [error])

  useEffect(() => () => clientRef.current?.disconnect(), [])

  const handleDisconnected = () => {
    setConnection('disconnected')
    setState(undefined)
    setTopSpeed(0)
    setBottomSpeed(0)
    setDeliverySpeed(0)
    pendingCountRef.current = 0
    setPendingCommand(false)
    setError('Bluetooth connection lost. The board has been stopped.')
  }

  const connect = async () => {
    if (!supported || connection === 'connecting') return
    setConnection('connecting')
    setError(undefined)

    const client = new TennisGunBleClient(handleDisconnected)
    clientRef.current = client
    try {
      const nextState = await client.connect()
      setState(nextState)
      setTopSpeed(nextState.top)
      setBottomSpeed(nextState.bottom)
      setDeliverySpeed(nextState.delivery)
      setConnection('connected')
      if (nextState.error) setError(`Board error: ${nextState.error}`)
    } catch (caughtError) {
      clientRef.current = undefined
      setConnection('disconnected')
      setError(caughtError instanceof Error ? caughtError.message : String(caughtError))
    }
  }

  const send = async (command: ControlCommand) => {
    if (connection !== 'connected') return
    pendingCountRef.current += 1
    setPendingCommand(true)
    try {
      const nextState = await clientRef.current?.send(command)
      if (!nextState) throw new Error('The board is not connected')
      setState(nextState)
      if (nextState.error) setError(`Board rejected the command: ${nextState.error}`)
    } catch (caughtError) {
      setError(caughtError instanceof Error ? caughtError.message : String(caughtError))
    } finally {
      pendingCountRef.current = Math.max(0, pendingCountRef.current - 1)
      setPendingCommand(pendingCountRef.current > 0)
    }
  }

  // BLE commands are serialized by the client. Keep the controls interactive while
  // a command is in flight; disabling them makes the whole panel appear frozen.
  const controlsDisabled = connection !== 'connected'
  const startButtonClass = state?.running ? 'btn-error' : state ? 'btn-primary' : 'btn-neutral'

  return (
    <main
      aria-busy={pendingCommand}
      className="flex min-h-dvh w-full flex-col items-center justify-center p-4 md:p-10"
    >
      {error && <ErrorToast message={error} onClose={() => setError(undefined)} />}

      <section className="mb-8 flex flex-col items-center gap-3 text-center">
        <p className="text-sm font-semibold uppercase tracking-widest text-gray-500">
          {connection === 'connected' ? 'Bluetooth connected' : 'Bluetooth disconnected'}
        </p>
        {connection !== 'connected' && (
          <button
            className="btn btn-primary btn-lg"
            disabled={!supported || connection === 'connecting'}
            onClick={connect}
            type="button"
          >
            {connection === 'connecting' ? 'Connecting…' : 'Connect Tennis Gun'}
          </button>
        )}
        {!supported && (
          <p className="max-w-xl text-red-700">
            Web Bluetooth is unavailable. Open this panel in Chrome or Edge on a supported Android, Windows, or macOS device.
          </p>
        )}
      </section>

      <button
        className={`btn btn-xl h-auto px-16 py-8 ${startButtonClass}`}
        disabled={controlsDisabled || !state}
        onClick={() => send({type: state?.running ? 'stop' : 'start'})}
        type="button"
      >
        {state ? (state.running ? 'Stop' : 'Start') : 'Unknown'}
      </button>

      <div className="mt-8 mb-12 flex h-[300px] flex-row items-center justify-center gap-6 sm:gap-16">
        <SpeedControl
          disabled={controlsDisabled}
          label="Shooter Top"
          onChange={setTopSpeed}
          onCommit={value => send({type: 'set_top_speed', value})}
          value={topSpeed}
        />
        <SpeedControl
          disabled={controlsDisabled}
          label="Shooter Bottom"
          onChange={setBottomSpeed}
          onCommit={value => send({type: 'set_bottom_speed', value})}
          value={bottomSpeed}
        />
        <SpeedControl
          disabled={controlsDisabled}
          label="Delivery"
          onChange={setDeliverySpeed}
          onCommit={value => send({type: 'set_delivery_speed', value})}
          value={deliverySpeed}
        />
      </div>

      <h2 className="mt-4 text-xl font-semibold">Free heap space: {state?.freeHeap ?? 0} KiB</h2>
    </main>
  )
}

export default App
