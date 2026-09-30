#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
APP_PATH="$DIR/BRAUN_MR16.app"
if [ ! -d "$APP_PATH" ]; then
    APP_PATH="$DIR/Standalone/BRAUN_MR16.app"
fi
if [ ! -d "$APP_PATH" ]; then
    APP_PATH="$HOME/Applications/BRAUN_MR16.app"
fi
if [ -d "$APP_PATH" ]; then
    xattr -cr "$APP_PATH" 2>/dev/null || true
    codesign --force --deep --sign - "$APP_PATH" 2>/dev/null || true
    open "$APP_PATH"
else
    echo "BRAUN_MR16.app not found. Please run Install.command first."
fi
