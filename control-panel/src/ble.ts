import {z} from 'zod'

export const SERVICE_UUID = 'f3641400-00b0-4240-ba50-05ca45bf8abc'
const CONTROL_UUID = 'f3641401-00b0-4240-ba50-05ca45bf8abc'
const STATUS_UUID = 'f3641402-00b0-4240-ba50-05ca45bf8abc'

const SpeedSchema = z.number().int().min(0).max(100)
const ControlErrorSchema = z.enum(['invalid_json', 'invalid_command', 'invalid_value']).nullable()

const DeviceStateSchema = z.object({
  running: z.boolean(),
  top: SpeedSchema,
  bottom: SpeedSchema,
  delivery: SpeedSchema,
  freeHeap: z.number().int().nonnegative(),
  error: ControlErrorSchema,
})

const ControlCommandSchema = z.discriminatedUnion('type', [
  z.object({type: z.literal('start')}),
  z.object({type: z.literal('stop')}),
  z.object({type: z.literal('set_top_speed'), value: SpeedSchema}),
  z.object({type: z.literal('set_bottom_speed'), value: SpeedSchema}),
  z.object({type: z.literal('set_delivery_speed'), value: SpeedSchema}),
])

export type ControlError = z.infer<typeof ControlErrorSchema>
export type DeviceState = z.infer<typeof DeviceStateSchema>
export type ControlCommand = z.infer<typeof ControlCommandSchema>

function parseStatus(value: DataView): DeviceState {
  const bytes = new Uint8Array(value.byteLength)
  bytes.set(new Uint8Array(value.buffer, value.byteOffset, value.byteLength))

  let parsedJson: unknown
  try {
    parsedJson = JSON.parse(new TextDecoder().decode(bytes))
  } catch {
    throw new Error('The board returned invalid status JSON')
  }

  const parsed = DeviceStateSchema.safeParse(parsedJson)
  if (!parsed.success) {
    throw new Error('The board returned an invalid status object')
  }

  return parsed.data
}

export function isWebBluetoothSupported() {
  return 'bluetooth' in navigator
}

export class TennisGunBleClient {
  private control?: BluetoothRemoteGATTCharacteristic
  private device?: BluetoothDevice
  private readonly onDisconnected: () => void
  private operationQueue: Promise<void> = Promise.resolve()
  private status?: BluetoothRemoteGATTCharacteristic

  constructor(onDisconnected: () => void) {
    this.onDisconnected = onDisconnected
  }

  async connect(): Promise<DeviceState> {
    if (!isWebBluetoothSupported()) {
      throw new Error('Web Bluetooth is not supported by this browser')
    }

    const device = await navigator.bluetooth.requestDevice({
      filters: [{services: [SERVICE_UUID]}],
    })
    this.device = device
    device.addEventListener('gattserverdisconnected', this.handleDisconnected)

    try {
      const server = await device.gatt?.connect()
      if (!server) throw new Error('The selected device has no GATT server')

      const service = await server.getPrimaryService(SERVICE_UUID)
      this.control = await service.getCharacteristic(CONTROL_UUID)
      this.status = await service.getCharacteristic(STATUS_UUID)
      return await this.readStatus()
    } catch (error) {
      device.removeEventListener('gattserverdisconnected', this.handleDisconnected)
      device.gatt?.disconnect()
      this.clearConnection()
      throw error
    }
  }

  async send(command: ControlCommand): Promise<DeviceState> {
    return this.enqueue(async () => {
      if (!this.control || !this.status || !this.device?.gatt?.connected) {
        throw new Error('The board is not connected')
      }

      const payload = new TextEncoder().encode(JSON.stringify(ControlCommandSchema.parse(command)))
      await this.control.writeValueWithResponse(payload)
      return this.readStatus()
    })
  }

  disconnect() {
    const device = this.device
    device?.removeEventListener('gattserverdisconnected', this.handleDisconnected)
    device?.gatt?.disconnect()
    this.clearConnection()
  }

  private readonly handleDisconnected = () => {
    this.clearConnection()
    this.onDisconnected()
  }

  private clearConnection() {
    this.control = undefined
    this.status = undefined
    this.device = undefined
    this.operationQueue = Promise.resolve()
  }

  private async readStatus(): Promise<DeviceState> {
    if (!this.status) throw new Error('Status characteristic is not available')
    return parseStatus(await this.status.readValue())
  }

  private enqueue<T>(operation: () => Promise<T>): Promise<T> {
    const result = this.operationQueue.then(operation)
    this.operationQueue = result.then(() => undefined, () => undefined)
    return result
  }
}
