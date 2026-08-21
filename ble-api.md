# Tennis Gun BLE API

The ESP32-C3 advertises as `Tennis Gun`. It accepts one unencrypted BLE connection.

## GATT service

| Item | UUID | Properties | Maximum value |
| --- | --- | --- | --- |
| Service | `f3641400-00b0-4240-ba50-05ca45bf8abc` | — | — |
| Control | `f3641401-00b0-4240-ba50-05ca45bf8abc` | Write with response | 128 bytes |
| Status | `f3641402-00b0-4240-ba50-05ca45bf8abc` | Read | 256 bytes |

The board requests an ATT MTU of 128. Both characteristics support standard GATT long-value operations when the negotiated payload is smaller than their JSON value.

## Control commands

Every control value is UTF-8 JSON with an explicit `type`. `start` and `stop` must contain no other fields. Speed commands must contain exactly `type` and an integer `value` from 0 through 100.

```json
{"type":"start"}
```

```json
{"type":"stop"}
```

```json
{"type":"set_top_speed","value":75}
```

```json
{"type":"set_bottom_speed","value":80}
```

```json
{"type":"set_delivery_speed","value":40}
```

## Status

Read status after connecting and after every control write.

```json
{
  "running": false,
  "top": 75,
  "bottom": 80,
  "delivery": 40,
  "freeHeap": 123,
  "error": null
}
```

`freeHeap` is measured in KiB. `error` is `null`, `invalid_json`, `invalid_command`, or `invalid_value`. A valid command clears the previous error.

When BLE disconnects, the board immediately sets `running` to false and stops all motor outputs. Configured speeds are preserved for the next connection. Physical link loss is detected according to the negotiated BLE supervision timeout; the board requests approximately two seconds.
