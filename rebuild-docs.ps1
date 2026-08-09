$ErrorActionPreference = "Stop"
Set-Location -Path $PSScriptRoot
mkdocs build --config-file ../mkdocs.yml
