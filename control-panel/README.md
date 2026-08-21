# Tennis Gun Control Panel

React control panel for the Tennis Gun ESP32-C3 board. It uses Web Bluetooth and is deployed as an offline-capable PWA on GitHub Pages.

## Browser support

Use Chrome or Edge on a Web Bluetooth capable Android, Windows, or macOS device. The initial device chooser must be opened by pressing the Connect button. Safari and Firefox are not supported.

## Development

```shell
npm ci
npm run dev
```

Web Bluetooth requires a secure context. `localhost` is treated as secure during local development.

## Verification

```shell
npm run lint
npm run build
```

Pushes to `main` run these checks and deploy `dist` to the `/tennis-gun/` GitHub Pages path.
