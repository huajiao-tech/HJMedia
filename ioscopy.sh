#!/bin/zsh

set -u

timestamp() {
  date "+%Y-%m-%d %H:%M:%S"
}

log_info() {
  echo "[$(timestamp)] [INFO] $1"
}

log_success() {
  echo "[$(timestamp)] [SUCCESS] $1"
}

log_error() {
  echo "[$(timestamp)] [ERROR] $1" >&2
}

usage() {
  cat <<'EOF'
用法:
  ./ioscopy.sh [Debug|Release]

说明:
  默认拷贝 Release 版本。
  传入 Debug 或 Release 可显式指定版本。
EOF
}

normalize_config() {
  local raw="${1:-Release}"
  case "$raw" in
    Debug|debug)
      echo "Debug"
      ;;
    Release|release)
      echo "Release"
      ;;
    -h|--help)
      usage >&2
      return 2
      ;;
    *)
      log_error "不支持的配置: $raw"
      usage >&2
      return 1
      ;;
  esac
}

copy_framework() {
  local src="$1"
  local dst_dir="${2%/}"
  local name
  name="$(basename "$src")"
  local dst="$dst_dir/$name"

  log_info "准备拷贝: $src -> $dst"

  if [ ! -e "$src" ]; then
    log_error "拷贝失败，源文件不存在: $src"
    return 1
  fi

  if [ ! -d "$dst_dir" ]; then
    log_info "目标目录不存在，准备创建: $dst_dir"
    if ! mkdir -p "$dst_dir"; then
      log_error "创建目标目录失败: $dst_dir"
      return 1
    fi
    log_success "目标目录创建成功: $dst_dir"
  fi

  if [ -e "$dst" ]; then
    log_info "目标已存在，先删除旧内容: $dst"
    if ! rm -rf "$dst"; then
      log_error "删除旧目标失败: $dst"
      return 1
    fi
    log_success "旧目标删除成功: $dst"
  fi

  if cp -R "$src" "$dst_dir/"; then
    log_success "拷贝成功: $src -> $dst"
    return 0
  fi

  log_error "拷贝失败: $src -> $dst"
  return 1
}

main() {
  local config
  local total=0
  local success=0
  local failed=0
  local hjmedia_root="/Users/lfs/code/git/hjmedia"
  local slmedia_root="/Users/lfs/code/git/slmedia"
  local huajiao_ios_root="/Users/lfs/code/git/huajiao_ios"
  local exit_code=0

  config="$(normalize_config "${1:-Release}")"
  exit_code=$?
  if [ "$exit_code" -ne 0 ]; then
    [ "$exit_code" -eq 2 ] && return 0
    return "$exit_code"
  fi

  log_info "当前拷贝配置: $config"

  total=$((total + 1))
  copy_framework \
    "$hjmedia_root/build_ios/out/libs/$config/HJInference.framework" \
    "$slmedia_root/Dependencies/ios/hjinference/" && success=$((success + 1)) || failed=$((failed + 1))

  total=$((total + 1))
  copy_framework \
    "$hjmedia_root/build_ios/out/libs/$config/HJInference.framework" \
    "$huajiao_ios_root/Pods/HJPlayer/HJPlayer/Framework" && success=$((success + 1)) || failed=$((failed + 1))

  total=$((total + 1))
  copy_framework \
    "$slmedia_root/build_ios/output/libs/$config/jplayer.framework" \
    "$huajiao_ios_root/Pods/HJPlayer/HJPlayer/Framework" && success=$((success + 1)) || failed=$((failed + 1))

  if [ "$failed" -eq 0 ]; then
    log_success "拷贝汇总: 总计 $total 次, 成功 $success 次, 失败 $failed 次"
    return 0
  fi

  log_error "拷贝汇总: 总计 $total 次, 成功 $success 次, 失败 $failed 次"
  return 1
}

main "$@"
