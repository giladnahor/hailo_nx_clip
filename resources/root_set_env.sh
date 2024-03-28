#!/bin/bash

# This script sets the environment variables required to run with Hailo Tappas
# It should be sourced from the mediaserver start script
# /opt/networkoptix-metavms/mediaserver/lib/scripts/systemd_mediaserver_start.sh
# Function to check if the script is being sourced
is_sourced() {
    [[ "${BASH_SOURCE[0]}" != "$0" ]]
}

# Only proceed if the script is being sourced
if is_sourced; then
    echo "Setting up the environment..."

    # Add Hailo pkgconfig path to PKG_CONFIG_PATH
    export PKG_CONFIG_PATH=/opt/hailo/tappas/pkgconfig:$PKG_CONFIG_PATH

    # Fetch the workspace path using pkg-config
    TAPPAS_WORKSPACE=$(pkg-config --variable=tappas_workspace hailo_tappas)

    if [ -z "$TAPPAS_WORKSPACE" ]; then
        echo "Error: TAPPAS_WORKSPACE could not be determined."
        echo "Make sure PKG_CONFIG_PATH includes /opt/hailo/tappas/pkgconfig/"
        return 1 # Use 'return' instead of 'exit' to not exit the user's shell
    fi

    # Export the environment variable so it's available to subprocesses
    export TAPPAS_WORKSPACE
    echo "TAPPAS_WORKSPACE set to $TAPPAS_WORKSPACE"

    # Set additional environment variables
    export LD_LIBRARY_PATH=/opt/hailo/tappas/lib/x86_64-linux-gnu:/usr/local/cuda-11.8/lib64:$LD_LIBRARY_PATH
    export GST_PLUGIN_PATH=/opt/hailo/tappas/lib/x86_64-linux-gnu/gstreamer-1.0

    # Activate the virtual environment
    VENV_PATH="${TAPPAS_WORKSPACE}/hailo_tappas_venv/bin/activate"
    if [ -f "$VENV_PATH" ]; then
        echo "Activating virtual environment..."
        source "$VENV_PATH"
    else
        echo "Error: Virtual environment not found at $VENV_PATH."
        return 1
    fi
else
    echo "This script needs to be sourced to correctly set up the environment. Please run '. $(basename "$0")' instead of executing it."
fi
