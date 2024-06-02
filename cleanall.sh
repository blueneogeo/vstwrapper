#!/bin/bash

echo "Cleaning all build dirs"

rm -rf Builds

for bank in {1..6}; do
    for slot in {1..12}; do
        PROJECT_NAME="ElectraB${bank}S${slot}"
        PRODUCT_NAME="ElectraOne bank ${bank} slot ${slot}"
        PLUGIN_CODE="EO$(printf "%X" $bank)$(printf "%X" $slot)"
        echo "=== === === CLEANING ${PRODUCT_NAME} === === ==="

        BUILD_DIR="BuildB${bank}S${slot}"
        rm -rf "${BUILD_DIR}"
    done
done

echo "Done"
