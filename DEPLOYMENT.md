# NEOBRICK web deployment

NEOBRICK is compiled with the repository's existing CMake `web-release` preset and
deployed as static files. Emscripten preloads the repository `assets/` directory into
`index.data`; no server-side runtime is required.

## Local Windows build

Install:

- Visual Studio or the C++ build tools
- CMake 3.24 or newer
- Ninja
- Python 3
- The [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

From PowerShell, activate the SDK in the same shell used for CMake:

```powershell
& C:\path\to\emsdk\emsdk_env.ps1
cmake --preset web-release
cmake --build --preset web-release
```

The static site is `build\web-release`. It must contain `index.html`, `index.js`,
`index.wasm`, and `index.data`. Do not commit this generated directory.

## Test locally over HTTP

From the repository root:

```powershell
python -m http.server 8080 --directory build\web-release
```

Open `http://localhost:8080/` in a browser. Use an HTTP server rather than opening
`index.html` directly: the browser must be able to fetch the JavaScript, WASM, and
preloaded data files.

## Create and connect the Vercel project

1. Sign in to Vercel and create a new project from the
   `Sanika7777/gameboy` GitHub repository.
2. Set the project framework preset to **Other**. The GitHub Actions workflow builds
   Emscripten output, so Vercel does not need to run a build.
3. The workflow deploys `build/web-release` directly with the Vercel CLI. Vercel's
   project dashboard may remain connected for visibility; disable its automatic
   Git deployments if you want GitHub Actions to be the only production deployer.
4. In the repository settings, open **Secrets and variables > Actions** and add
   these repository secrets:

   - `VERCEL_TOKEN`: a Vercel personal or team access token created in Vercel
     Account Settings > Tokens.
   - `VERCEL_ORG_ID`: the Vercel team ID (or personal account ID) from the project's
     settings or the Vercel project API.
   - `VERCEL_PROJECT_ID`: the project's ID from Vercel Project Settings > General.

Pushes to `main` and manually dispatched runs then build and deploy production.
Pull requests do not deploy production. The workflow also uploads the generated
site as the `neobrick-web` Actions artifact for diagnostics.

## Troubleshooting

- **CMake cannot find Emscripten:** run `emsdk_env.ps1` again in the current
  PowerShell session and confirm `Get-Command emcmake, emcc`.
- **Missing output files:** remove only `build\web-release` and repeat the two
  preset commands. Check that the Emscripten SDK version is available and that
  `assets\` is present.
- **WASM or data fetch errors:** serve `build\web-release` through HTTP, not
  `file://`. The committed `vercel.json` sets `application/wasm` for `.wasm`
  responses; inspect the browser Network panel for `index.wasm` and `index.data`.
- **404s for assets:** do not add a SPA rewrite. Runtime files are packed into
  `index.data` at `/assets`, as configured by `CMakeLists.txt`.
- **Vercel authentication or project errors:** verify all three exact secret names,
  that the token can deploy to the selected team/project, and that the IDs belong
  to the same Vercel project. The workflow fails before deployment when a secret
  is empty.
- **Workflow does not run:** production deployment is intentionally limited to
  `main`; use `workflow_dispatch` for a manual production run.
