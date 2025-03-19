# PowerShell 스크립트 (파일명: convert-to-utf8.ps1)
$sourceDir = ".."
$extensions = @("*.cpp", "*.h", "*.hpp")

foreach ($ext in $extensions) {
    Get-ChildItem -Path $sourceDir -Filter $ext -Recurse | ForEach-Object {
        $content = Get-Content -Path $_.FullName -Raw
        $utf8NoBom = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($_.FullName, $content, $utf8NoBom)
        Write-Host "Converted to UTF-8: $($_.FullName)"
    }
}