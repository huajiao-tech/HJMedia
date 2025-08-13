#!/bin/bash

projs=("sonic" "soundtouch" "librtmp" "yyjson" "fdk-aac" "spdlog")

for i in "${!projs[@]}"; do
    target_dir="third_party/${projs[$i]}"
    patch_file="patches/${projs[$i]}.patch"

    echo "checking patch: $patch_file"
    if git apply --check --directory="$target_dir" "$patch_file" &>/dev/null; then
        echo "applying patch: $patch_file"
        if git apply --directory="$target_dir" "$patch_file" &>/dev/null; then
            echo "apply patch success"
        else
            echo "apply patch failed"
            exit 1
        fi
    else
        echo "skipping: $patch_file already applied"
    fi
    echo
done