#!/bin/bash
# ==============================================================================
# BRAUN MR-16 — Easy Installer for macOS (Universal: Apple Silicon & Intel)
# Double-click this script in Finder to install AU, VST3, CLAP, and Standalone.
# ==============================================================================

set -e

# Set working directory to script location
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# ANSI color codes
BOLD='\033[1m'
CYAN='\033[0;36m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
RESET='\033[0m'

echo ""
echo -e "${BOLD}${ORANGE}================================================================${RESET}"
echo -e "${BOLD}${ORANGE}     BRAUN MR-16 MODAL RESONATOR — macOS EASY INSTALLER        ${RESET}"
echo -e "${BOLD}${ORANGE}================================================================${RESET}"
echo ""

ARCH=$(uname -m)
echo -e "${CYAN}Target System Architecture:${RESET} ${BOLD}${ARCH}${RESET}"
echo ""

# Destination directories in user Library (no root / sudo required)
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
CLAP_DIR="$HOME/Library/Audio/Plug-Ins/CLAP"
APP_DIR="$HOME/Applications"

echo "Creating plugin destination directories..."
mkdir -p "$AU_DIR" "$VST3_DIR" "$CLAP_DIR" "$APP_DIR"

# 1. Install Audio Unit (.component)
AU_SRC=$(find "$DIR" -maxdepth 2 -name "BRAUN_MR16.component" -type d | head -n 1 || true)
if [ -n "$AU_SRC" ] && [ -d "$AU_SRC" ]; then
    echo -e "Installing ${BOLD}Audio Unit (AU)${RESET} -> $AU_DIR/BRAUN_MR16.component..."
    rm -rf "$AU_DIR/BRAUN_MR16.component"
    cp -R "$AU_SRC" "$AU_DIR/"
    xattr -cr "$AU_DIR/BRAUN_MR16.component" 2>/dev/null || true
    codesign --force --deep --sign - "$AU_DIR/BRAUN_MR16.component" 2>/dev/null || true
    echo -e "${GREEN}  ✓ AU installed, signed, and unquarantined${RESET}"
fi

# 2. Install VST3 (.vst3)
VST3_SRC=$(find "$DIR" -maxdepth 2 -name "BRAUN_MR16.vst3" -type d | head -n 1 || true)
if [ -n "$VST3_SRC" ] && [ -d "$VST3_SRC" ]; then
    echo -e "Installing ${BOLD}VST3${RESET} -> $VST3_DIR/BRAUN_MR16.vst3..."
    rm -rf "$VST3_DIR/BRAUN_MR16.vst3"
    cp -R "$VST3_SRC" "$VST3_DIR/"
    xattr -cr "$VST3_DIR/BRAUN_MR16.vst3" 2>/dev/null || true
    codesign --force --deep --sign - "$VST3_DIR/BRAUN_MR16.vst3" 2>/dev/null || true
    echo -e "${GREEN}  ✓ VST3 installed, signed, and unquarantined${RESET}"
fi

# 3. Install CLAP (.clap)
CLAP_SRC=$(find "$DIR" -maxdepth 2 -name "BRAUN_MR16.clap" | head -n 1 || true)
if [ -n "$CLAP_SRC" ] && [ -e "$CLAP_SRC" ]; then
    echo -e "Installing ${BOLD}CLAP${RESET} -> $CLAP_DIR/BRAUN_MR16.clap..."
    rm -rf "$CLAP_DIR/BRAUN_MR16.clap"
    cp -R "$CLAP_SRC" "$CLAP_DIR/"
    xattr -cr "$CLAP_DIR/BRAUN_MR16.clap" 2>/dev/null || true
    codesign --force --deep --sign - "$CLAP_DIR/BRAUN_MR16.clap" 2>/dev/null || true
    echo -e "${GREEN}  ✓ CLAP installed, signed, and unquarantined${RESET}"
fi

# 4. Install Standalone (.app)
APP_SRC=$(find "$DIR" -maxdepth 2 -name "BRAUN_MR16.app" -type d | head -n 1 || true)
if [ -n "$APP_SRC" ] && [ -d "$APP_SRC" ]; then
    echo -e "Installing ${BOLD}Standalone App${RESET} -> $APP_DIR/BRAUN_MR16.app..."
    rm -rf "$APP_DIR/BRAUN_MR16.app"
    cp -R "$APP_SRC" "$APP_DIR/"
    xattr -cr "$APP_DIR/BRAUN_MR16.app" 2>/dev/null || true
    codesign --force --deep --sign - "$APP_DIR/BRAUN_MR16.app" 2>/dev/null || true
    echo -e "${GREEN}  ✓ Standalone app installed, signed, and unquarantined${RESET}"
fi

# 5. Flush macOS AudioComponentRegistrar cache so Logic / DAWs see it immediately
echo ""
echo "Flushing macOS CoreAudio plugin cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
echo -e "${BOLD}${GREEN}================================================================${RESET}"
echo -e "${BOLD}${GREEN}         INSTALLATION COMPLETE — BRAUN MR-16 IS READY!          ${RESET}"
echo -e "${BOLD}${GREEN}================================================================${RESET}"
echo ""
echo "DAW Availability:"
echo "  • Logic Pro / GarageBand:  Audio Units -> Braun -> BRAUN_MR16"
echo "  • Ableton Live / FL Studio: VST3 -> Braun -> BRAUN_MR16"
echo "  • Reaper / Bitwig / Studio One: CLAP or VST3 -> BRAUN_MR16"
echo "  • Standalone: Launch from $APP_DIR/BRAUN_MR16.app"
echo ""
read -n 1 -s -r -p "Press any key to close this window..."
echo ""
