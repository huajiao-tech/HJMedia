#!/bin/bash

# 脚本出错时立即退出
set -e

# 获取git仓库的根目录，确保脚本可以在任何路径下执行
GIT_ROOT=$(git rev-parse --show-toplevel)
cd "$GIT_ROOT"

projs=("sonic" "soundtouch" "librtmp" "yyjson" "fdk-aac" "spdlog" "imgui" "implot" "glfw" "ImFileDialog" "ImGuiFileDialog" "zlib" "lz4")

echo "Starting patch application process..."
echo "====================================="

for proj in "${projs[@]}"; do
    target_dir="third_party/$proj"
    patch_file="patches/$proj.patch"

    echo
    echo ">>> Processing project: $proj"

    if [ ! -f "$patch_file" ]; then
        echo "  [WARNING] Patch file not found, skipping: $patch_file"
        continue
    fi

    # 使用 git apply --reverse --check 来判断补丁是否已经应用
    # 如果此命令成功，意味着补丁可以被反向应用，即已经应用过了
    if git apply --reverse --check --directory="$target_dir" "$patch_file" &>/dev/null; then
        echo "  [INFO] Patch already applied, skipping."
        continue
    fi

    # 补丁未应用，现在尝试应用它
    # 首先，进行预检查（--check），并捕获错误输出
    echo "  [INFO] Checking patch..."
    if ! apply_check_output=$(git apply --check --directory="$target_dir" "$patch_file" 2>&1); then
        echo "  [ERROR] Patch cannot be applied cleanly for '$proj'."
        echo "  -------------------- GIT APPLY --CHECK FAILED --------------------"
        echo "$apply_check_output"
        echo "  ----------------------------------------------------------------"
        exit 1
    fi

    # 预检查通过，正式应用补丁
    echo "  [INFO] Applying patch..."
    if ! git apply --directory="$target_dir" "$patch_file"; then
        # 正常情况下，如果--check通过，这里不应该失败，但作为兜底
        echo "  [ERROR] An unexpected error occurred while applying patch for '$proj'."
        exit 1
    fi
    
    echo "  [SUCCESS] Patch applied successfully for '$proj'."
done

echo
echo "====================================="
echo "All patches processed successfully."