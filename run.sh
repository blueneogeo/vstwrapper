
# cmake --build Builds --config Debug
# open Builds/VstWrapper_artefacts/Standalone/VstWrapper.app

#!/bin/bash

cmake --build Builds --config Release

# Check if cmake build was successful
if [ $? -eq 0 ]; then
    open Builds/VstWrapper_artefacts/Standalone/VstWrapper.app
else
    echo "Build failed."
fi
