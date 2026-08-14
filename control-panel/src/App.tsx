import {useEffect, useState, type KeyboardEvent} from 'react'

type State = {
  running: boolean
  freeHeap: number
  shooter: {
    top_speed: number
    bottom_speed: number
  }
  delivery: {
    speed: number
  }
}

type SpeedControlProps = {
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
  'ArrowDown',
  'ArrowLeft',
  'ArrowRight',
  'ArrowUp',
  'End',
  'Home',
  'PageDown',
  'PageUp',
])

function SpeedControl({label, value, onChange, onCommit}: SpeedControlProps) {
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

  const handleKeyUp = (event: KeyboardEvent<HTMLInputElement>) => {
    if (COMMIT_KEYS.has(event.key)) onCommit(event.currentTarget.valueAsNumber)
  }

  return (
    <section className="flex h-full flex-col items-center">
      <label className="mb-4 text-center text-xl font-semibold" htmlFor={`speed-${label}`}>
        {label} <span className="inline-block min-w-16">{value}%</span>
      </label>
      <input
        aria-valuetext={`${value}%`}
        className="range range-primary range-xl range-vertical h-full"
        id={`speed-${label}`}
        max={100}
        min={0}
        onInput={event => onChange(event.currentTarget.valueAsNumber)}
        onKeyDown={handleKeyDown}
        onKeyUp={handleKeyUp}
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

async function request(path: string, method: 'POST' | 'PATCH') {
  const response = await fetch(path, {method})
  if (!response.ok) throw new Error(`${method} ${path} failed with status ${response.status}`)
}

function App() {
  const [state, setState] = useState<State>()
  const [error, setError] = useState<string>()
  const [pendingRun, setPendingRun] = useState(false)
  const [shooterBottomSpeed, setShooterBottomSpeed] = useState(0)
  const [shooterTopSpeed, setShooterTopSpeed] = useState(0)
  const [deliverySpeed, setDeliverySpeed] = useState(0)

  useEffect(() => {
    setShooterTopSpeed(state?.shooter.top_speed ?? 0)
  }, [state?.shooter.top_speed])

  useEffect(() => {
    setShooterBottomSpeed(state?.shooter.bottom_speed ?? 0)
  }, [state?.shooter.bottom_speed])

  useEffect(() => {
    setDeliverySpeed(state?.delivery.speed ?? 0)
  }, [state?.delivery.speed])

  useEffect(() => {
    if (!error) return

    const timeout = window.setTimeout(() => setError(undefined), 5000)
    return () => window.clearTimeout(timeout)
  }, [error])

  useEffect(() => {
    const source = new EventSource('/status-events')
    source.onerror = event => {
      console.error(event)
      setError('status-events error')
    }
    source.onmessage = event => {
      setState(JSON.parse(event.data))
    }
    return () => source.close()
  }, [])

  const runAction = async (path: '/start' | '/stop') => {
    if (pendingRun) return

    setPendingRun(true)
    try {
      await request(path, 'POST')
    } catch (caughtError) {
      console.error(caughtError)
      setError(caughtError instanceof Error ? caughtError.message : String(caughtError))
    } finally {
      setPendingRun(false)
    }
  }

  const updateSpeed = async (path: string) => {
    try {
      await request(path, 'PATCH')
    } catch (caughtError) {
      console.error(caughtError)
      setError(caughtError instanceof Error ? caughtError.message : String(caughtError))
    }
  }

  const startButtonClass = state?.running ? 'btn-error' : state ? 'btn-primary' : 'btn-neutral'

  return (
    <main className="flex h-dvh w-full flex-col items-center justify-center p-4 md:p-10">
      {error && <ErrorToast message={error} onClose={() => setError(undefined)} />}

      <button
        className={`btn btn-xl h-auto px-16 py-8 ${startButtonClass}`}
        disabled={!state || pendingRun}
        onClick={() => runAction(state?.running ? '/stop' : '/start')}
        type="button"
      >
        {state ? (state.running ? 'Stop' : 'Start') : 'Unknown'}
      </button>

      <div className="mt-8 mb-12 flex h-[300px] flex-row items-center justify-center gap-16">
        <SpeedControl
          label="Shooter Top"
          onChange={setShooterTopSpeed}
          onCommit={value => updateSpeed(`/shooter?${new URLSearchParams({top_speed: String(value)})}`)}
          value={shooterTopSpeed}
        />
        <SpeedControl
          label="Shooter Bottom"
          onChange={setShooterBottomSpeed}
          onCommit={value => updateSpeed(`/shooter?${new URLSearchParams({bottom_speed: String(value)})}`)}
          value={shooterBottomSpeed}
        />
        <SpeedControl
          label="Delivery"
          onChange={setDeliverySpeed}
          onCommit={value => updateSpeed(`/delivery?${new URLSearchParams({speed: String(value)})}`)}
          value={deliverySpeed}
        />
      </div>

      <h2 className="mt-4 text-xl font-semibold">Free heap space: {state?.freeHeap ?? 0}</h2>
    </main>
  )
}

export default App
