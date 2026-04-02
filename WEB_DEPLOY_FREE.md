# Free Hosting For `BloodyGoodCurry`

`build_web_html.bat` already produces a static web build, so we do not need a paid game server.

The catch is the generated asset bundle is large:

- `build\web-release-split\Sandbox\BloodyGoodCurry.data` is about `268 MB`

That is too large for several "easy" static hosts, so the safest free setup for this repo is:

- `Cloudflare Pages` for `index.html`, `.js`, and `.wasm`
- `Cloudflare R2` for the big `.data` file

This repo now includes:

- [`app_resources/web/index_shell.html`](/c:/Users/justin/source/csd2401f25_team_sofasqud/app_resources/web/index_shell.html)
  The web shell can load the `.data` file from `window.SOFASPUDS_DATA_BASE_URL` or `?dataBaseUrl=...`
- [`tools/prepare_cloudflare_web_deploy.ps1`](/c:/Users/justin/source/csd2401f25_team_sofasqud/tools/prepare_cloudflare_web_deploy.ps1)
  Prepares separate `Pages` and `R2` upload folders from the existing web build

## 1. Rebuild The Web Version

From the repo root:

```powershell
build_web_html.bat BloodyGoodCurry release-split reconfigure noserve
```

## 2. Prepare The Upload Folders

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\prepare_cloudflare_web_deploy.ps1
```

This creates:

- `deploy\cloudflare\pages`
- `deploy\cloudflare\r2`

## 3. Upload The Big Data File To R2

In Cloudflare:

1. Create an `R2` bucket, for example `bloodygoodcurry-data`
2. Enable a public URL for the bucket
3. Upload `deploy\cloudflare\r2\BloodyGoodCurry.data`
4. Copy the public base URL

Example of the value you want in `site-config.js`:

```js
window.SOFASPUDS_DATA_BASE_URL = 'https://pub-xxxxxxxxxxxxxxxx.r2.dev';
```

Then update:

- [`deploy\cloudflare\pages\site-config.js`](/c:/Users/justin/source/csd2401f25_team_sofasqud/deploy/cloudflare/pages/site-config.js)

Or rerun the prep script with the URL already filled in:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\prepare_cloudflare_web_deploy.ps1 -DataBaseUrl "https://pub-xxxxxxxxxxxxxxxx.r2.dev"
```

## 4. Upload The Site To Pages

In Cloudflare Pages:

1. Create a project named `bloodygoodcurry` if it is available
2. Use direct upload
3. Upload the contents of `deploy\cloudflare\pages`

If the project name is available, your free public URL should be:

- `https://bloodygoodcurry.pages.dev`

If that exact name is taken, use the closest free variant such as:

- `https://bloodygoodcurry-game.pages.dev`
- `https://bloodygoodcurry-web.pages.dev`

## 5. Important Domain Note

`BloodyGoodCurry` by itself is not a free domain name. A bare custom domain normally has to be registered and paid for.

Free options:

- `bloodygoodcurry.pages.dev`
- another free subdomain on a hosting provider

Paid option:

- buy a real domain like `bloodygoodcurry.com`, then connect it to Cloudflare Pages

## 6. Quick Test Without Editing Files

Because the HTML shell now accepts a query parameter, you can also test a remote data URL like this:

```text
https://bloodygoodcurry.pages.dev/?dataBaseUrl=https://pub-xxxxxxxxxxxxxxxx.r2.dev
```
