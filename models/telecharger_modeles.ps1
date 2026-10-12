# Télécharge les deux modèles OpenCV Zoo nécessaires à la reconnaissance faciale (Windows).
# Clic droit > Exécuter avec PowerShell, ou :  powershell -ExecutionPolicy Bypass -File telecharger_modeles.ps1
Set-Location $PSScriptRoot
$base = "https://github.com/opencv/opencv_zoo/raw/main/models"
function Get-Modele($nom, $dossier, $tailleMin, $obligatoire) {
    Write-Host "Téléchargement de $nom ..."
    try { Invoke-WebRequest -Uri "$base/$dossier/$nom" -OutFile $nom -UseBasicParsing } catch {}
    if ((-not (Test-Path $nom)) -or ((Get-Item $nom).Length -lt $tailleMin)) {
        Remove-Item $nom -ErrorAction SilentlyContinue
        if ($obligatoire) { Write-Host "ECHEC : $nom"; exit 1 } else { Write-Host "  (facultatif, ignoré)" }
    } else { Write-Host "  OK" }
}
Get-Modele "face_detection_yunet_2023mar.onnx"   "face_detection_yunet"   100000  $true
Get-Modele "face_recognition_sface_2021dec.onnx" "face_recognition_sface" 1000000 $true
Get-Modele "face_detection_yunet_2022mar.onnx"   "face_detection_yunet"   100000  $false
Write-Host "Terminé."
