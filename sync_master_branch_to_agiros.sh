#!/bin/bash
set -e

# --- Configuration ---
AGIROS_DEBIAN_DIR="debian-agiros-loong"
AGIROS_RPM_DIR="rpm-agiros-loong"
TARGET_DEBIAN_DIR="debian"
TARGET_RPM_DIR="rpm"
MASTER_DEBIAN_DIR="debian-master"
MASTER_RPM_DIR="rpm-master"

ROOT_SPEC_FILE=$(basename $(find "./" -maxdepth 1 -name "*.spec" | head -n 1))
SCRIPT_NAME="sync_master_branch_to_agiros.sh"  # 明确指定只忽略此脚本

# --- Functions ---

print_header() {
    echo "=== 开始同步 Master 分支到 Master-AGIROS ==="
}

check_spec_file() {
    if [ -z "$ROOT_SPEC_FILE" ]; then
        echo "错误: 在当前目录或 $TARGET_RPM_DIR 中未找到 .spec 文件，无法确定根 spec 文件名。"
        exit 1
    fi
    echo "检测到 Spec 文件: $ROOT_SPEC_FILE"
}

parse_args() {
    CONTINUE_MODE=false
    while getopts "c" opt; do
      case $opt in
        c)
          CONTINUE_MODE=true
          echo "模式: 继续执行 (跳过同步和覆盖步骤)"
          ;;
        \?)
          echo "无效选项: -$OPTARG" >&2
          exit 1
          ;;
      esac
    done
}

ensure_branch() {
    local branch="master-agiros"
    CURRENT_BRANCH=$(git symbolic-ref --short HEAD)

    if [ "$CURRENT_BRANCH" != "$branch" ]; then
        echo "当前在 $CURRENT_BRANCH 分支，正在切换到 $branch 分支..."
        if ! git checkout -f "$branch"; then
            echo "错误: 无法切换到 $branch 分支。请检查该分支是否存在。"
            exit 1
        fi
        CURRENT_BRANCH=$(git symbolic-ref --short HEAD)
        echo "当前在 $CURRENT_BRANCH 分支"
    fi
}

check_workspace_clean() {
    # 获取git状态，只过滤掉脚本自身
    local git_status_output
    git_status_output=$(git status --porcelain)
    
    # 检查是否有非本脚本的更改
    local has_other_changes=false
    while IFS= read -r line; do
        # 提取文件名（去除状态前缀）
        local file_status="${line:0:2}"
        local file_name="${line:3}"
        
        # 跳过空行和本脚本自身
        if [ -z "$line" ] || [[ "$file_name" == "$SCRIPT_NAME" ]]; then
            continue
        fi
        
        has_other_changes=true
        break
    done <<< "$git_status_output"
    
    if [ "$has_other_changes" = true ]; then
        echo "错误: 工作区不干净，存在未提交的更改。"
        echo "========================================================"
        echo "当前仓库远程: $(git remote get-url origin)"
        echo "当前分支: $CURRENT_BRANCH"
        echo "--------------------------------------------------------"
        echo "不干净文件状态 (已过滤脚本 '$SCRIPT_NAME'):"
        # 显示除本脚本外的所有状态
        while IFS= read -r line; do
            local file_name="${line:3}"
            if [[ "$file_name" != "$SCRIPT_NAME" ]]; then
                echo "$line"
            fi
        done <<< "$git_status_output"
        echo "========================================================"
        echo "请选择以下一种方式处理："
        echo "1. 提交更改: git add . && git commit -m '...'"
        echo "2. 暂存更改: git stash (同步完成后使用 git stash pop 恢复)"
        echo "3. 仅忽略脚本文件继续: 手动暂存脚本文件: git add $SCRIPT_NAME"
        echo "   或运行: git add $SCRIPT_NAME && $0"
        exit 1
    fi
}

perform_resume_checks() {
    echo "--------------------------------------------------------"
    echo "重入模式预检查:"

    # 1. 分支检查
    if [ "$CURRENT_BRANCH" = "master-agiros" ]; then
        echo "[OK] 当前分支: $CURRENT_BRANCH"
    else
        echo "[FAIL] 当前分支: $CURRENT_BRANCH (预期: master-agiros)"
        exit 1
    fi

    # 2. 冲突检查
    UNRESOLVED=$(git diff --name-only --diff-filter=U)
    if [ -z "$UNRESOLVED" ]; then
        echo "[OK] 冲突状态: 所有文件已标记为解决"
    else
        echo "[FAIL] 冲突状态: 检测到未解决的冲突！"
        echo "$UNRESOLVED"
        echo "请先解决冲突并执行 'git add <file>'。"
        exit 1
    fi

    # 3. 提交状态预判
    if [ -f .git/MERGE_HEAD ]; then
         echo "[OK] 合并状态: 正在进行合并 (MERGE_HEAD 存在)"
    else
         echo "[INFO] 合并状态: 普通提交模式 (无正在进行的合并)"
    fi
    echo "--------------------------------------------------------"
}

perform_merge() {
    echo "正在从 master 分支合并..."
    git merge --no-commit master || echo "警告: 自动合并失败 (存在冲突)，将继续执行配置覆盖..."
}

clean_configs_dir() {
    echo "重命名master-agiros分支rpm和debian目录..."

    # Backup master version
    #echo "正在创建 debian-master 和 rpm-master 备份..."
    rm -rf "$MASTER_DEBIAN_DIR"
    rm -rf "$MASTER_RPM_DIR"
    rm -rf "$TARGET_DEBIAN_DIR"
    rm -rf "$TARGET_RPM_DIR"
}

apply_configs() {
    echo "正在从 master 分支提取agiros-loong打包配置..."

    # Reset packaging dirs to master state
    # 同时检出 agiros-loong 目录，确保它们存在（因为 master-agiros 分支本身不应该保留它们）
    git checkout master -- "$TARGET_DEBIAN_DIR" "$TARGET_RPM_DIR" "$AGIROS_DEBIAN_DIR" "$AGIROS_RPM_DIR"

    # Backup master version
    echo "正在创建 debian-master 和 rpm-master 备份..."
    mv "$TARGET_DEBIAN_DIR" "$MASTER_DEBIAN_DIR"
    mv "$TARGET_RPM_DIR" "$MASTER_RPM_DIR"

    # Overwrite with agiros version
    echo "正在从 $AGIROS_DEBIAN_DIR 和 $AGIROS_RPM_DIR 覆盖配置..."
    mv "$AGIROS_DEBIAN_DIR" "$TARGET_DEBIAN_DIR"
    mv "$AGIROS_RPM_DIR" "$TARGET_RPM_DIR"

    if [ -f "$TARGET_RPM_DIR/$ROOT_SPEC_FILE" ]; then
        cp "$TARGET_RPM_DIR/$ROOT_SPEC_FILE" .
    else
        echo "警告: $TARGET_RPM_DIR/$ROOT_SPEC_FILE 不存在，无法覆盖根 spec 文件。"
	exit 1
    fi
}

cleanup_dirs() {
    echo "清理tar包文件..."

    # 使用master备份的spec文件
    local spec_file="$MASTER_RPM_DIR/$ROOT_SPEC_FILE"

    if [ ! -f "$spec_file" ]; then
        echo " 备份的spec文件不存在。"
        exit 1
    fi

    # 检查spec文件中是否有Source0定义
    if ! grep -q "^Source0:" "$spec_file"; then
        echo "提示: spec文件中未定义Source0，无法进行tar包清理。"
        exit 1
    fi

    # 安全地解析tar包名
    local tar_name=""
    tar_name=$(grep "^Source0:" "$spec_file" | head -n 1)

    if [ -n "$tar_name" ]; then
        # 提取Name和Version
        local spec_name=$(grep '^Name:' "$spec_file" | head -n 1 | awk '{print $2}')
        local spec_version=$(grep '^Version:' "$spec_file" | head -n 1 | awk '{print $2}')

        # 处理tar包名
        tar_name=$(echo "$tar_name" | sed 's/Source0:[[:space:]]*//')
        tar_name="${tar_name//%\{name\}/$spec_name}"
        tar_name="${tar_name//%\{version\}/$spec_version}"
        tar_name=$(basename "$tar_name" 2>/dev/null || echo "")
    fi

    # 删除文件
    if [ -n "$tar_name" ] && [ -f "$tar_name" ]; then
        echo "删除: $tar_name"
        rm -f "$tar_name"
    elif [ -z "$tar_name" ]; then
        echo "警告: 无法解析tar包文件名"
    else
        echo "tar包不存在: $tar_name"
    fi

    return 0
}

cleanup_dirs1() {
    echo "清理tar包文件..."

    # 使用master备份的spec文件
    local spec_file="$MASTER_RPM_DIR/$ROOT_SPEC_FILE"

    if [ ! -f "$spec_file" ]; then
        echo "跳过: 备份的spec文件不存在。"
        return 0
    fi

    # 解析tar包名
    local tar_name=$(
        grep "^Source0:" "$spec_file" | head -n 1 | \
        sed "s/Source0:[[:space:]]*//" | \
        sed "s/%{name\}/$(grep '^Name:' "$spec_file" | awk '{print $2}')/" | \
        sed "s/%{version\}/$(grep '^Version:' "$spec_file" | awk '{print $2}')/" | \
        basename
    )

    # 删除文件
    if [ -n "$tar_name" ] && [ -f "$tar_name" ]; then
        echo "删除: $tar_name"
        rm -f "$tar_name"
    fi

    return 0
}

stage_files() {
    # 再次add所有相关文件，确保覆盖和解决后的内容被暂存
    # 同时 add 被删除的 agiros 目录以记录删除操作
    git add "$TARGET_DEBIAN_DIR" "$TARGET_RPM_DIR" "$ROOT_SPEC_FILE" "$MASTER_DEBIAN_DIR" "$MASTER_RPM_DIR"
    git add "$AGIROS_DEBIAN_DIR" "$AGIROS_RPM_DIR" || true # 将删除操作加入暂存区 (如果目录已删除并提交，忽略错误)
}

check_remaining_conflicts() {
    if [ -n "$(git diff --name-only --diff-filter=U)" ]; then
        echo "========================================================"
        echo "警告: 检测到代码文件存在冲突！"
        echo "打包配置文件已自动更新并备份，但以下文件需要人工解决："
        git diff --name-only --diff-filter=U
        echo "========================================================"
        echo "请手动解决上述文件的冲突 (git add <file>)，"
        echo "确认所有冲突都已解决并暂存后，请运行以下命令继续："
        echo "  $0 -c"
        exit 1
    fi
}

generate_package_and_commit() {
    local spec_file=$1

    # 1. 检查是否有 Source0 定义 (忽略注释)
    # 格式示例: Source0: %{name}-%{version}.tar.gz
    local source0_line=$(grep "^Source0:" "$spec_file" | head -n 1 | awk '{$1=""; print $0}' | sed 's/^[ \t]*//')

    if [ -z "$source0_line" ]; then
        echo "提示: Spec 文件未启用 Source0 (被注释或不存在)，跳过源码包生成。"
        return 0
    fi

    echo "正在根据 Source0 定义生成源码包..."
    local spec_name=$(grep -i "^Name:" "$spec_file" | awk '{print $2}')
    local spec_version=$(grep -i "^Version:" "$spec_file" | awk '{print $2}')

    if [ -z "$spec_name" ] || [ -z "$spec_version" ]; then
        echo "错误: 无法从 $spec_file 解析 Name 或 Version。"
        return 1
    fi

    # 2. 解析 Source0 文件名中的宏
    # 将 %{name} 替换为实际名称，%{version} 替换为实际版本
    local tarball_name="$source0_line"
    tarball_name="${tarball_name//%\{name\}/$spec_name}"
    tarball_name="${tarball_name//%\{version\}/$spec_version}"

    # 去除可能存在的 URL 前缀 (只取文件名)
    tarball_name=$(basename "$tarball_name")

    # --- 生成并提交源码包 ---
    echo "生成并提交源码包: $tarball_name"
    # 先把刚提交的 commit 的状态拿来生成 tarball
    git archive --format=tar.gz --prefix="${spec_name}-${spec_version}/" HEAD -o "$tarball_name"
    git add "$tarball_name"
    # 修改 Commit Message 以反映 tarball 的加入
    git commit --amend -m "Merge master into master-agiros (kept agiros-loong configs, backup master configs, update tarball)"
    # ---------------------------
}

finalize_commit() {
    local git_status_output
    git_status_output=$(git status --porcelain)
    
    # 检查是否有非本脚本的更改
    local changes_excluding_script=false
    while IFS= read -r line; do
        local file_name="${line:3}"
        if [[ "$file_name" != "$SCRIPT_NAME" ]]; then
            changes_excluding_script=true
            break
        fi
    done <<< "$git_status_output"
    
    if [ "$changes_excluding_script" = true ]; then
        echo "准备提交..."
        git commit -m "Merge master into master-agiros (kept agiros-loong configs, backup master configs)"

        # 尝试生成源码包 (如果 Spec 中定义了 Source0)
        # 如果不需要生成源码包，请注释掉下面行代码
        generate_package_and_commit "$ROOT_SPEC_FILE" || exit 1

        echo "=== 同步完成！ ==="
        echo "准备推送到远程的内容:"
        echo "--------------------------------------------------------"
        # 显示将要推送的 commit
        git log --oneline --graph --decorate origin/"$CURRENT_BRANCH"..HEAD
        echo "--------------------------------------------------------"
        # 显示文件变化统计
        git diff --stat origin/"$CURRENT_BRANCH"..HEAD
        echo "--------------------------------------------------------"
        echo "如果需要回退本次操作，请执行: git reset --hard HEAD^"
        echo "可以使用 'git push' 推送到远程仓库。"
    elif [ -n "$git_status_output" ]; then
        echo "=== 只有本脚本文件 '$SCRIPT_NAME' 有更改，没有其他变更需要提交。 ==="
        echo "提示: 本脚本文件有未提交更改，但不会影响同步结果。"
        echo "      如果需要提交脚本文件，请手动执行: git add $SCRIPT_NAME && git commit -m 'update script'"
    else
        echo "=== 没有检测到任何变更，已经是最新状态。 ==="
    fi
}

# --- Main Execution Flow ---

# 1. 前置检查与环境准备
check_spec_file
print_header
parse_args "$@"
ensure_branch

# 2. 核心逻辑执行
if [ "$CONTINUE_MODE" = false ]; then
    # 普通模式：检查工作区 -> 合并分支 -> 应用配置 -> 清理临时目录
    check_workspace_clean
    clean_configs_dir
    perform_merge
    apply_configs
    cleanup_dirs
else
    # 重入模式：执行状态检查（分支、冲突、合并状态）
    perform_resume_checks
fi

# 3. 收尾工作
# 暂存所有变更 -> 检查剩余冲突 -> 提交变更（可选打包）
stage_files
check_remaining_conflicts
finalize_commit
