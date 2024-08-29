#!/bin/bash
# setup tappas env
. ./setup_env.sh

# get clip-app directory
CLIP_APP_DIR=$(pip show clip-app | grep Location | awk '{print $2}')

# Set the environment variable
export CLIP_APP_DIR="$CLIP_APP_DIR"


export CLIP_RESOURCES_DIR=$CLIP_APP_DIR/resources/

# Verify the environment variable is set
echo coping data from clip resources $CLIP_RESOURCES_DIR

# CLIP resources
cp -r $CLIP_RESOURCES_DIR/* .

# TAPPAS resources
cp $TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/libyolo_post.so .
cp $TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/cropping_algorithms/libwhole_buffer.so .

echo "Remember to copy the resources directory to nx plugin directory!!"
