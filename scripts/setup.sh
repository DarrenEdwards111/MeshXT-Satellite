#!/usr/bin/env bash
set -euo pipefail

echo "=== MeshXT-Satellite Setup ==="
echo ""

# Check PlatformIO
if ! command -v pio &> /dev/null; then
    echo "→ Installing PlatformIO..."
    pip install platformio
fi

echo "→ Installing PlatformIO dependencies..."
pio pkg install

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  1. Edit src/gateway/config.h with your LoRaWAN keys"
echo "  2. Connect your device via USB"
echo "  3. Flash: bash scripts/flash.sh heltec-v3-gateway"
echo ""
