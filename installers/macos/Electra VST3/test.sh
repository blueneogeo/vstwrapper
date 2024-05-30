ORIGINAL_FILE="/Library/Audio/Plug-Ins/VST3/ElectraOne________________________________________.vst3"

if ! stat "$ORIGINAL_FILE" >/dev/null 2>&1; then
    echo "Original Electra One VST3 plugin not found at $ORIGINAL_FILE!"
    exit 1
else
    echo "Original Electra One VST3 plugin found at $ORIGINAL_FILE."
fi
