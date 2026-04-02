param(
  [string]$GameName = "BloodyGoodCurry",
  [string]$Preset = "web-release-split",
  [string]$OutputRoot = "deploy\\netlify",
  [int]$ChunkSizeMB = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RepoRoot {
  return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Ensure-InsideRoot {
  param(
    [Parameter(Mandatory = $true)][string]$RootPath,
    [Parameter(Mandatory = $true)][string]$CandidatePath
  )

  $resolvedRoot = [System.IO.Path]::GetFullPath($RootPath)
  $resolvedCandidate = [System.IO.Path]::GetFullPath($CandidatePath)
  if (-not $resolvedRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
    $resolvedRoot += [System.IO.Path]::DirectorySeparatorChar
  }

  if (-not $resolvedCandidate.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to operate outside repo root: $resolvedCandidate"
  }

  return $resolvedCandidate
}

function Reset-Directory {
  param(
    [Parameter(Mandatory = $true)][string]$RepoRoot,
    [Parameter(Mandatory = $true)][string]$TargetPath
  )

  $safeTarget = Ensure-InsideRoot -RootPath $RepoRoot -CandidatePath $TargetPath
  if (Test-Path -LiteralPath $safeTarget) {
    Remove-Item -LiteralPath $safeTarget -Recurse -Force
  }
  New-Item -ItemType Directory -Path $safeTarget -Force | Out-Null
}

function Split-FileIntoChunks {
  param(
    [Parameter(Mandatory = $true)][string]$SourcePath,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][string]$BaseFileName,
    [Parameter(Mandatory = $true)][int]$ChunkSizeBytes
  )

  $chunkNames = [System.Collections.Generic.List[string]]::new()
  $buffer = New-Object byte[] $ChunkSizeBytes
  $stream = [System.IO.File]::OpenRead($SourcePath)
  try {
    $chunkIndex = 0
    while (($bytesRead = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
      $chunkName = "{0}.part{1:D3}" -f $BaseFileName, $chunkIndex
      $chunkPath = Join-Path $OutputDirectory $chunkName
      $chunkBytes = New-Object byte[] $bytesRead
      [System.Array]::Copy($buffer, 0, $chunkBytes, 0, $bytesRead)
      [System.IO.File]::WriteAllBytes($chunkPath, $chunkBytes)
      $chunkNames.Add($chunkName)
      $chunkIndex++
    }
  } finally {
    $stream.Dispose()
  }

  return $chunkNames.ToArray()
}

$repoRoot = Get-RepoRoot
$buildDir = Join-Path $repoRoot "build\$Preset\Sandbox"
$outputRootAbs = Ensure-InsideRoot -RootPath $repoRoot -CandidatePath (Join-Path $repoRoot $OutputRoot)
$siteDir = Join-Path $outputRootAbs "site"
$dataDir = Join-Path $siteDir "data"

$sourceHtml = Join-Path $buildDir "$GameName.html"
$sourceJs = Join-Path $buildDir "$GameName.js"
$sourceWasm = Join-Path $buildDir "$GameName.wasm"
$sourceData = Join-Path $buildDir "$GameName.data"

foreach ($requiredPath in @($sourceHtml, $sourceJs, $sourceWasm, $sourceData)) {
  if (-not (Test-Path -LiteralPath $requiredPath)) {
    throw "Missing build output: $requiredPath`nRun build_web_html.bat $GameName $Preset noserve first."
  }
}

if ($ChunkSizeMB -lt 1 -or $ChunkSizeMB -gt 9) {
  throw "ChunkSizeMB must stay between 1 and 9 for Netlify-safe chunks."
}

Reset-Directory -RepoRoot $repoRoot -TargetPath $siteDir
New-Item -ItemType Directory -Path $dataDir -Force | Out-Null

$siteIndex = Join-Path $siteDir "index.html"
$siteGameHtml = Join-Path $siteDir "$GameName.html"
$siteJs = Join-Path $siteDir "$GameName.js"
$siteWasm = Join-Path $siteDir "$GameName.wasm"
$siteConfig = Join-Path $siteDir "site-config.js"
$siteHeaders = Join-Path $siteDir "_headers"

$html = Get-Content -LiteralPath $sourceHtml -Raw
$scriptTagPattern = "<script>"
$scriptTagReplacement = "<script src=""site-config.js""></script><script>"
$scriptTagRegex = [regex]$scriptTagPattern
$patchedHtml = $scriptTagRegex.Replace($html, $scriptTagReplacement, 1)

if ($patchedHtml -eq $html) {
  throw "Failed to inject site-config.js into $sourceHtml"
}

$js = Get-Content -LiteralPath $sourceJs -Raw
$fetchMarker = 'async function fetchRemotePackage(packageName,packageSize){'
$chunkFetchLogic = @'
const sofaChunkConfig=globalThis["SOFASPUDS_DATA_CHUNKS"];if(sofaChunkConfig&&Array.isArray(sofaChunkConfig.files)&&sofaChunkConfig.files.length){const sofaBaseUrl=String(sofaChunkConfig.baseUrl||"").replace(/\/+$/,"");const sofaJoinUrl=(baseUrl,fileName)=>baseUrl?baseUrl+"/"+String(fileName).replace(/^\/+/,""):String(fileName);if(!Module["dataFileDownloads"])Module["dataFileDownloads"]={};const chunkBuffers=[];let totalLoaded=0;const totalExpected=Number(sofaChunkConfig.totalBytes||packageSize||0);Module["setStatus"]&&Module["setStatus"](`Downloading data... (0/${totalExpected||"?"})`);for(const chunkName of sofaChunkConfig.files){const chunkUrl=sofaJoinUrl(sofaBaseUrl,chunkName);let chunkResponse;try{chunkResponse=await fetch(chunkUrl)}catch(e){throw new Error(`Network Error: ${chunkUrl}`)}if(!chunkResponse.ok){throw new Error(`${chunkResponse.status}: ${chunkResponse.url}`)}const chunkArray=new Uint8Array(await chunkResponse.arrayBuffer());chunkBuffers.push(chunkArray);totalLoaded+=chunkArray.length;Module["dataFileDownloads"][chunkUrl]={loaded:chunkArray.length,total:chunkArray.length};Module["setStatus"]&&Module["setStatus"](`Downloading data... (${totalLoaded}/${totalExpected||totalLoaded})`)}const packageData=new Uint8Array(totalLoaded);let packageOffset=0;for(const chunkArray of chunkBuffers){packageData.set(chunkArray,packageOffset);packageOffset+=chunkArray.length}return packageData.buffer}
'@
$chunkFetchLogic = $fetchMarker + $chunkFetchLogic
$patchedJs = $js.Replace($fetchMarker, $chunkFetchLogic)

if ($patchedJs -eq $js) {
  throw "Failed to patch the Emscripten data loader in $sourceJs"
}

$chunkSizeBytes = $ChunkSizeMB * 1MB
$chunkNames = Split-FileIntoChunks -SourcePath $sourceData -OutputDirectory $dataDir -BaseFileName "$GameName.data" -ChunkSizeBytes $chunkSizeBytes
$chunkCount = $chunkNames.Count
$chunkConfig = @{
  baseUrl = "./data"
  totalBytes = (Get-Item -LiteralPath $sourceData).Length
  files = $chunkNames
}
$chunkConfigJson = $chunkConfig | ConvertTo-Json -Compress
$configBody = @(
  "// Generated by tools/prepare_netlify_web_deploy.ps1",
  "window.SOFASPUDS_DATA_CHUNKS = $chunkConfigJson;",
  ""
) -join [Environment]::NewLine

$headersBody = @(
  "/*",
  "  Cache-Control: public, max-age=0, must-revalidate",
  "",
  "/*.js",
  "  Cache-Control: public, max-age=31536000, immutable",
  "",
  "/*.wasm",
  "  Content-Type: application/wasm",
  "  Cache-Control: public, max-age=31536000, immutable",
  "",
  "/data/*",
  "  Content-Type: application/octet-stream",
  "  Cache-Control: public, max-age=31536000, immutable",
  ""
) -join [Environment]::NewLine

Set-Content -LiteralPath $siteIndex -Value $patchedHtml
Set-Content -LiteralPath $siteGameHtml -Value $patchedHtml
Set-Content -LiteralPath $siteJs -Value $patchedJs
Set-Content -LiteralPath $siteConfig -Value $configBody
Set-Content -LiteralPath $siteHeaders -Value $headersBody
Copy-Item -LiteralPath $sourceWasm -Destination $siteWasm

$largestChunkMb = [math]::Round(((Get-ChildItem -LiteralPath $dataDir | Measure-Object -Maximum Length).Maximum / 1MB), 2)
$dataSizeMb = [math]::Round(((Get-Item -LiteralPath $sourceData).Length / 1MB), 2)

Write-Host "Prepared Netlify deploy folder:"
Write-Host "  Site: $siteDir"
Write-Host ""
Write-Host "Files:"
Write-Host "  index.html"
Write-Host "  $GameName.html"
Write-Host "  $GameName.js"
Write-Host "  $GameName.wasm"
Write-Host "  site-config.js"
Write-Host "  data\\*.partNNN"
Write-Host ""
Write-Host "Split $GameName.data ($dataSizeMb MB) into $chunkCount chunks."
Write-Host "Largest chunk: $largestChunkMb MB"
Write-Host ""
Write-Host "Upload the contents of '$siteDir' to Netlify Drop or a Netlify site."
