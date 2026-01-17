# Generate self-signed certificate for localhost testing
# Run this script to create cert.pem and key.pem

$openssl = "C:\Program Files\Git\usr\bin\openssl.exe"

if (-not (Test-Path $openssl)) {
    $openssl = "openssl"  # Try PATH
}

# Generate private key and certificate in one command
& $openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes `
    -subj "/CN=localhost/O=Coroute Test/C=US" `
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nCertificate generated successfully!" -ForegroundColor Green
    Write-Host "Files created:"
    Write-Host "  - cert.pem (certificate)"
    Write-Host "  - key.pem (private key)"
    Write-Host "`nTo run HTTPS server:"
    Write-Host "  ..\build\v2\examples\Release\https_server.exe cert.pem key.pem"
    Write-Host "`nNote: Browsers will show a security warning for self-signed certs."
    Write-Host "      Click 'Advanced' -> 'Proceed to localhost' to continue."
} else {
    Write-Host "Failed to generate certificate" -ForegroundColor Red
}
