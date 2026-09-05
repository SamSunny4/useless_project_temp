# TinkerHub Auto-Uploader Script
Write-Host "=========================================================="
Write-Host " TINKERHUB AUTO-UPLOADER: WAITING FOR ESP32 CONNECTION... "
Write-Host "=========================================================="
Write-Host "Ready to flash 360-Degree Evasion Firmware!"
Write-Host "1. Connect your PC Wi-Fi to 'ESP32-EvadeBot-AP' (password: admin12345)"
Write-Host "   OR"
Write-Host "2. Plug in the ESP32 USB cable into your PC"
Write-Host "----------------------------------------------------------"

$binPath = "e:\TinkerHub\.pio\build\esp32doit-devkit-v1\firmware.bin"
$espotaPath = "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32\tools\espota.py"
$pioPath = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"

$startTime = Get-Date
while ($true) {
    # Check 1: WiFi OTA (192.168.4.1)
    $wifiOk = Test-Connection -ComputerName 192.168.4.1 -Count 1 -Quiet -ErrorAction SilentlyContinue
    if ($wifiOk) {
        Write-Host "`n>>> [DETECTED] ESP32 AP reachable at 192.168.4.1! Starting wireless OTA flash..." -ForegroundColor Green
        $hostIp = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias "Wi-Fi*" -ErrorAction SilentlyContinue | Where-Object { $_.IPAddress -like "192.168.4.*" }).IPAddress
        if (-not $hostIp) { $hostIp = "192.168.4.2" }
        
        Write-Host "Flashing using Host IP: $hostIp -> ESP32: 192.168.4.1"
        & python $espotaPath -i 192.168.4.1 -I $hostIp -p 3232 -a admin -f $binPath -r -d
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n>>> [SUCCESS] FIRMWARE FLASHED OVER-THE-AIR SUCCESSFULLY! <<<" -ForegroundColor Green
            exit 0
        } else {
            Write-Host "OTA flash attempt returned code $LASTEXITCODE. Retrying..." -ForegroundColor Yellow
        }
    }

    # Check 2: USB COM port
    $ports = [System.IO.Ports.SerialPort]::getportnames()
    if ($ports -and $ports.Count -gt 0) {
        $port = $ports[0]
        Write-Host "`n>>> [DETECTED] ESP32 connected on $port! Starting USB Serial flash..." -ForegroundColor Green
        & $pioPath run -t upload --upload-port $port
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n>>> [SUCCESS] FIRMWARE FLASHED VIA USB SERIAL SUCCESSFULLY! <<<" -ForegroundColor Green
            exit 0
        }
    }

    Start-Sleep -Seconds 2
    if ((Get-Date) - $startTime -gt (New-TimeSpan -Minutes 10)) {
        Write-Host "Timeout waiting for ESP32 connection."
        exit 1
    }
}
