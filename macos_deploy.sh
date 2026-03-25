#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

AIN_IMAGER_APP="$SCRIPT_DIR/ain_imager_src/ain_imager.app"
AIN_VIEWER_APP="$SCRIPT_DIR/ain_viewer_src/ain_viewer.app"
LZ4_LIB="$SCRIPT_DIR/external/lz4/liblz4.1.dylib"
INDIGO_LIB="$SCRIPT_DIR/../indigo/build/lib"
MACDEPLOYQT="$HOME/Qt/6.11.0/macos/bin/macdeployqt"
BONJOUR_SERVICES=("_indigo._tcp")
BUNDLE_IDS=("com.yourcompany.ain-imager" "com.yourcompany.ain-viewer")
SIGNING_IDENTITY="${SIGNING_IDENTITY:-Ain Local Sign}"
APP_EXECUTABLES=("ain_imager" "ain_viewer")
APP_ICONS=("appicon.icns" "ain_viewer.icns")
SIGNING_MODE="adhoc"

resolve_signing_mode() {
  if security find-identity -v -p codesigning | grep -Fq "$SIGNING_IDENTITY"; then
    SIGNING_MODE="identity"
    echo "Using code signing identity: $SIGNING_IDENTITY"
  else
    SIGNING_MODE="adhoc"
    echo "Warning: missing code signing identity: $SIGNING_IDENTITY"
    echo "Falling back to ad-hoc signing. Both apps will be deployed, but Local Network permission may not persist across rebuilds."
  fi
}

# Copy liblz4 into app Frameworks and fix all references to it
bundle_lz4() {
  APP_PATH=$1
  BINARY="$APP_PATH/Contents/MacOS/$(basename "$APP_PATH" .app)"
  FRAMEWORKS_DIR="$APP_PATH/Contents/Frameworks"
  BUNDLED_LZ4="@executable_path/../Frameworks/liblz4.1.dylib"

  echo "Bundling liblz4 into $APP_PATH"
  mkdir -p "$FRAMEWORKS_DIR"
  cp "$LZ4_LIB" "$FRAMEWORKS_DIR/liblz4.1.dylib"
  install_name_tool -id "$BUNDLED_LZ4" "$FRAMEWORKS_DIR/liblz4.1.dylib"

  # Fix any absolute or /usr/local reference in the binary
  for OLD_PATH in \
      "/usr/local/lib/liblz4.1.dylib" \
      "$LZ4_LIB"; do
    if otool -L "$BINARY" | grep -q "$OLD_PATH"; then
      install_name_tool -change "$OLD_PATH" "$BUNDLED_LZ4" "$BINARY"
    fi
  done
}

# Patch the freshly built app before macdeployqt scans it.
# This avoids the first-run error on clean builds where the binary still points to /usr/local/lib/liblz4.1.dylib.
prepare_lz4_for_macdeployqt() {
  APP_PATH=$1
  BINARY="$APP_PATH/Contents/MacOS/$(basename "$APP_PATH" .app)"

  if [ -f "$BINARY" ]; then
    bundle_lz4 "$APP_PATH"
  fi
}

# Re-sign the entire app bundle (required after install_name_tool modifications on Apple Silicon)
resign_app() {
  APP_PATH=$1
  echo "Re-signing $APP_PATH"
  # Sign all dylibs/frameworks inside the bundle first, then the app itself
  find "$APP_PATH/Contents/Frameworks" -name "*.dylib" -o -name "*.framework" | while read -r lib; do
    if [ "$SIGNING_MODE" = "identity" ]; then
      codesign --force --sign "$SIGNING_IDENTITY" "$lib" 2>/dev/null
    else
      codesign --force --sign - "$lib" 2>/dev/null
    fi
  done
  if [ "$SIGNING_MODE" = "identity" ]; then
    codesign --force --deep --sign "$SIGNING_IDENTITY" "$APP_PATH"
  else
    codesign --force --deep --sign - "$APP_PATH"
  fi
}

# Patch Info.plist with local network privacy keys required by macOS 14+
# Without these the OS silently blocks TCP connections to discovered Bonjour services
patch_info_plist() {
  APP_PATH=$1
  BUNDLE_ID=$2
  EXECUTABLE=$3
  ICON_FILE=$4
  PLIST="$APP_PATH/Contents/Info.plist"
  echo "Patching Info.plist for local network access in $APP_PATH"

  /usr/libexec/PlistBuddy -c "Add :CFBundleExecutable string '$EXECUTABLE'" "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :CFBundleExecutable '$EXECUTABLE'" "$PLIST"

  /usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string '$ICON_FILE'" "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :CFBundleIconFile '$ICON_FILE'" "$PLIST"

  /usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string '$BUNDLE_ID'" "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier '$BUNDLE_ID'" "$PLIST"

  /usr/libexec/PlistBuddy -c "Add :CFBundlePackageType string 'APPL'" "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :CFBundlePackageType 'APPL'" "$PLIST"

  /usr/libexec/PlistBuddy -c "Add :NSPrincipalClass string 'NSApplication'" "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :NSPrincipalClass 'NSApplication'" "$PLIST"

  # NSLocalNetworkUsageDescription - shown to the user in the permission dialog
  /usr/libexec/PlistBuddy -c \
    "Add :NSLocalNetworkUsageDescription string 'Required to connect to INDIGO astronomy devices on the local network.'" \
    "$PLIST" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c \
    "Set :NSLocalNetworkUsageDescription 'Required to connect to INDIGO astronomy devices on the local network.'" \
    "$PLIST"

  # NSBonjourServices - list every Bonjour service type the app browses/connects to
  /usr/libexec/PlistBuddy -c "Remove :NSBonjourServices" "$PLIST" 2>/dev/null
  /usr/libexec/PlistBuddy -c "Add :NSBonjourServices array" "$PLIST"
  for i in "${!BONJOUR_SERVICES[@]}"; do
    /usr/libexec/PlistBuddy -c "Add :NSBonjourServices:${i} string '${BONJOUR_SERVICES[$i]}'" "$PLIST"
  done
}

resolve_signing_mode

# Deploy ain_imager
prepare_lz4_for_macdeployqt "$AIN_IMAGER_APP"
$MACDEPLOYQT "$AIN_IMAGER_APP" -libpath="$INDIGO_LIB" -libpath="$SCRIPT_DIR/external/lz4/"
bundle_lz4 "$AIN_IMAGER_APP"
patch_info_plist "$AIN_IMAGER_APP" "${BUNDLE_IDS[0]}" "${APP_EXECUTABLES[0]}" "${APP_ICONS[0]}"
resign_app "$AIN_IMAGER_APP"

# Deploy ain_viewer
prepare_lz4_for_macdeployqt "$AIN_VIEWER_APP"
$MACDEPLOYQT "$AIN_VIEWER_APP" -libpath="$INDIGO_LIB" -libpath="$SCRIPT_DIR/external/lz4/"
bundle_lz4 "$AIN_VIEWER_APP"
patch_info_plist "$AIN_VIEWER_APP" "${BUNDLE_IDS[1]}" "${APP_EXECUTABLES[1]}" "${APP_ICONS[1]}"
resign_app "$AIN_VIEWER_APP"

# Verify deployment
echo "Verifying ain_imager deployment"
otool -L "$AIN_IMAGER_APP/Contents/MacOS/ain_imager"

echo "Verifying ain_viewer deployment"
otool -L "$AIN_VIEWER_APP/Contents/MacOS/ain_viewer"