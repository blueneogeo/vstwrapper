#!/bin/bash
USER_INPUT_FILE="/tmp/electravst3.txt"

rm $USER_INPUT_FILE

USER_INPUT=$(osascript <<EOD
tell application "System Events"
    activate
    set user_input to text returned of (display dialog "What is the name of the VST you want to have your Electra One control?" default answer "")
end tell
EOD)

echo "$USER_INPUT" > $USER_INPUT_FILE

exit 0

