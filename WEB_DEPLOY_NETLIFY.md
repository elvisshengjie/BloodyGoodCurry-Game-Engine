# Netlify Free Deploy Notes

Netlify Free can host the HTML, JS, and wasm for this game, but the original web package contains one very large file:

- `build\web-release-split\Sandbox\BloodyGoodCurry.data` is about `268 MB`

Netlify's docs say files larger than `10 MB` are not well-supported by their CDN, so this repo includes a Netlify prep script that splits the `.data` file into smaller chunks before upload.

Files added for this flow:

- [`tools/prepare_netlify_web_deploy.ps1`](/c:/Users/justin/source/csd2401f25_team_sofasqud/tools/prepare_netlify_web_deploy.ps1)
- [`WEB_DEPLOY_NETLIFY.md`](/c:/Users/justin/source/csd2401f25_team_sofasqud/WEB_DEPLOY_NETLIFY.md)

## 1. Build The Web Version

```powershell
build_web_html.bat BloodyGoodCurry release-split reconfigure noserve
```

## 2. Prepare A Netlify Upload Folder

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\prepare_netlify_web_deploy.ps1
```

This creates:

- `deploy\netlify\site`

Inside that folder:

- the HTML page
- the JS loader
- the wasm file
- `site-config.js`
- a `data` folder full of small `.partNNN` chunk files

## 3. Upload To Netlify

Use Netlify Drop:

- https://app.netlify.com/drop

Drag the contents of `deploy\netlify\site` into Netlify Drop, or create a normal Netlify site and upload that folder.

If Netlify Drop stalls, use the CLI instead:

```powershell
npx netlify-cli login
npx netlify-cli deploy --dir=deploy\netlify\site --prod
```

## 4. Why The Chunking Exists

The prep script patches the generated Emscripten loader so it reassembles the split files in the browser and feeds the original virtual package back into the game at startup.

That means:

- Netlify only receives small static files
- the game still sees one logical `BloodyGoodCurry.data` package at runtime
