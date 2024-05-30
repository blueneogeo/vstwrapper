#!/bin/bash

# Define the path to the user input file
USER_INPUT_FILE="/tmp/electravst3.txt"

# Check if the user input file exists
if [ ! -f "$USER_INPUT_FILE" ]; then
    echo "User input file not found!"
    exit 1
fi

# Read the user input from the file
USER_INPUT=$(cat "$USER_INPUT_FILE")

# Ensure USER_INPUT is exactly 50 characters long (same as "ElectraOne")
if [[ ${#USER_INPUT} -gt 50 ]]; then
    echo "User input too long! Must be 50 characters or less."
    exit 1
fi

# Pad USER_INPUT with spaces if it's shorter than 10 characters
PADDED_USER_INPUT=$(printf "%-50s" "$USER_INPUT")

# Define the original and new file paths
ORIGINAL_FILE="/Library/Audio/Plug-Ins/VST3/ElectraOne.vst3"
NEW_FILE="/Library/Audio/Plug-Ins/VST3/${USER_INPUT}.vst3"

# Check if the original file exists
if ! stat "$ORIGINAL_FILE" >/dev/null 2>&1; then
    echo "Original Electra One VST3 plugin not found at $ORIGINAL_FILE!"
    exit 1
else
    echo "Original Electra One VST3 plugin found at $ORIGINAL_FILE."
fi

# Rename the original file to the new name based on user input
mv "$ORIGINAL_FILE" "$NEW_FILE"

# Check if renaming was successful and provide feedback
if [ $? -ne 0 ]; then
    echo "Failed to rename $ORIGINAL_FILE"
    exit 1
fi

echo "Renamed $ORIGINAL_FILE to $NEW_FILE"

COMPANY_DOMAIN="com.sagittarian"
RANDOM_STRING=$(LC_ALL=C tr -dc 'a-z0-9' </dev/urandom | head -c 8)
NEW_BUNDLE_IDENTIFIER="$COMPANY_DOMAIN.vst$RANDOM_STRING"
NEW_BUNDLE_NAME=$USER_INPUT
INFO_PLIST="$NEW_FILE/Contents/Info.plist"

/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $NEW_BUNDLE_IDENTIFIER" "$INFO_PLIST"
/usr/libexec/PlistBuddy -c "Set :CFBundleName $NEW_BUNDLE_NAME" "$INFO_PLIST"
/usr/libexec/PlistBuddy -c "Set :CFBundleDisplayName $NEW_BUNDLE_NAME" "$INFO_PLIST"

echo "Updated Plist $INFO_PLIST"

echo "Done"

exit 0
