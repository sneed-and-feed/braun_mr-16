#!/bin/bash
set -e
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
CLAP_DIR="$HOME/Library/Audio/Plug-Ins/CLAP"
APP_DIR="$HOME/Applications"

echo "Uninstalling BRAUN MR-16..."
rm -rf "$AU_DIR/BRAUN_MR16.component"
rm -rf "$VST3_DIR/BRAUN_MR16.vst3"
rm -rf "$CLAP_DIR/BRAUN_MR16.clap"
rm -rf "$APP_DIR/BRAUN_MR16.app"

killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "BRAUN MR-16 has been removed."
read -n 1 -s -r -p "Press any key to close this window..."
echo ""
