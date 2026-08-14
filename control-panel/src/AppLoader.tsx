import {useEffect, useState} from 'react'
import App from './App.tsx'

const MDNS_HOSTNAME = 'tennis-gun.local'

type LoadState =
  | {status: 'loading'}
  | {status: 'ready'}
  | {status: 'error', message: string}

type IpResponse = {
  ip: string
}

function isIpv4Address(value: unknown): value is string {
  if (typeof value !== 'string') return false

  const octets = value.split('.')
  return octets.length === 4 && octets.every(octet => {
    if (!/^\d{1,3}$/.test(octet)) return false
    const value = Number(octet)
    return value >= 0 && value <= 255
  })
}

function isIpResponse(value: unknown): value is IpResponse {
  return typeof value === 'object'
    && value !== null
    && 'ip' in value
    && isIpv4Address(value.ip)
}

function isMdnsHost() {
  return window.location.hostname.toLowerCase().replace(/\.$/, '') === MDNS_HOSTNAME
}

async function redirectToIp() {
  const response = await fetch('/ip', {
    headers: {Accept: 'application/json'},
  })
  if (!response.ok) {
    throw new Error(`IP request failed with status ${response.status}`)
  }

  const body: unknown = await response.json()
  if (!isIpResponse(body)) {
    throw new Error('The board returned an invalid IPv4 address')
  }

  const target = new URL(window.location.href)
  target.hostname = body.ip
  window.location.replace(target.href)
}

function AppLoader() {
  const [loadState, setLoadState] = useState<LoadState>(() => (
    isMdnsHost() ? {status: 'loading'} : {status: 'ready'}
  ))

  useEffect(() => {
    if (loadState.status !== 'loading') return

    redirectToIp().catch(error => {
      const message = error instanceof Error ? error.message : String(error)
      setLoadState({status: 'error', message})
    })
  }, [loadState.status])

  if (loadState.status === 'ready') return <App />

  if (loadState.status === 'error') {
    return (
      <main className="h-dvh w-full flex flex-col items-center justify-center p-4 text-center">
        <h1 className="text-2xl font-semibold">Failed to determine the board IP address</h1>
        <p className="mt-4 text-gray-600">{loadState.message}</p>
      </main>
    )
  }

  return (
    <main className="h-dvh w-full flex items-center justify-center p-4 text-center">
      <p className="text-gray-600">Determining the board IP address...</p>
    </main>
  )
}

export default AppLoader
