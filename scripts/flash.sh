#!/usr/bin/env bash
set -euo pipefail

TARGET="${1:-}"

if [ -z "$TARGET" ]; then
    echo "MeshXT-Satellite Flash Tool"
    echo ""
    echo "Usage: bash scripts/flash.sh <board>"
    echo ""
    echo "Boards:"
    echo "  heltec-v3-gateway    Heltec WiFi LoRa 32 V3"
    echo "  tbeam-gateway        LilyGO T-Beam"
    echo "  rak4631-gateway      RAK WisBlock 4631"
    echo ""
    exit 1
fi

echo "→ Building and flashing: $TARGET"
pio run -e "$TARGET" --target upload
echo "✓ Flashed successfully"
