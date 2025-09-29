#!/bin/bash
# persistence_service_watchdog.sh - 持久化服务守护脚本
# 
# 用法: ./persistence_service_watchdog.sh [domain_id] [database_path] [--gui]
# 示例:
#   ./persistence_service_watchdog.sh
#   ./persistence_service_watchdog.sh 0 enhanced_persistence.db
#   ./persistence_service_watchdog.sh --gui

# 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "$SCRIPT_DIR/../build" && pwd)"
SERVICE_PATH="$BUILD_DIR/persistence_service_app"
LOG_FILE="$BUILD_DIR/persistence_watchdog.log"

# 守护参数
MAX_RAPID_RESTARTS=3      # 快速重启次数阈值
RAPID_RESTART_WINDOW=60   # 快速重启窗口时间(秒)
RESTART_DELAY=5           # 普通重启等待时间
BACKOFF_DELAY=60          # 快速重启后的退避时间

# 传递所有命令行参数
SERVICE_ARGS="$@"

# 全局变量
SERVICE_PID=""
RUNNING=true

# 显示使用说明
show_usage() {
    echo "持久化服务守护脚本"
    echo "用法: $0 [参数]"
    echo ""
    echo "参数:"
    echo "  domain_id database_path  指定域ID和数据库路径"
    echo "  --gui                    启动GUI模式"
    echo "  --help                   显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                                    # 使用默认参数"
    echo "  $0 0 enhanced_persistence.db         # 指定域和数据库"
    echo "  $0 --gui                             # GUI模式"
    echo ""
    echo "日志文件: $LOG_FILE"
}

# 日志函数
log() {
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] $1" | tee -a "$LOG_FILE"
}

# 检查依赖
check_dependencies() {
    if [ ! -f "$SERVICE_PATH" ]; then
        log "错误: 找不到持久化服务可执行文件: $SERVICE_PATH"
        log "请确保已经编译了项目"
        exit 1
    fi
    
    if [ ! -x "$SERVICE_PATH" ]; then
        log "错误: 持久化服务文件没有执行权限: $SERVICE_PATH"
        log "尝试添加执行权限..."
        chmod +x "$SERVICE_PATH" 2>/dev/null
        if [ $? -ne 0 ]; then
            log "无法添加执行权限，请手动执行: chmod +x $SERVICE_PATH"
            exit 1
        fi
        log "已添加执行权限"
    fi
}

# 优雅退出处理
cleanup() {
    log "守护脚本接收到终止信号，正在关闭..."
    RUNNING=false
    
    # 如果服务还在运行，发送SIGTERM信号给它
    if [ -n "$SERVICE_PID" ] && kill -0 $SERVICE_PID 2>/dev/null; then
        log "正在终止持久化服务 (PID: $SERVICE_PID)"
        kill -TERM $SERVICE_PID 2>/dev/null
        
        # 等待最多10秒让服务优雅退出
        local wait_count=0
        while [ $wait_count -lt 10 ] && kill -0 $SERVICE_PID 2>/dev/null; do
            sleep 1
            wait_count=$((wait_count + 1))
        done
        
        # 如果仍未退出，强制终止
        if kill -0 $SERVICE_PID 2>/dev/null; then
            log "服务未在10秒内退出，强制终止"
            kill -KILL $SERVICE_PID 2>/dev/null
        fi
        
        log "持久化服务已终止"
    fi
    
    log "守护脚本已关闭"
    exit 0
}

# 检查服务健康状态
check_service_health() {
    if [ -z "$SERVICE_PID" ]; then
        return 1
    fi
    
    # 检查进程是否存在
    if ! kill -0 $SERVICE_PID 2>/dev/null; then
        return 1
    fi
    
    return 0
}

# 等待服务退出并返回退出码
wait_for_service() {
    local exit_code=0
    
    # 使用wait命令等待子进程
    wait $SERVICE_PID 2>/dev/null
    exit_code=$?
    
    return $exit_code
}

# 启动服务
start_service() {
    log "启动持久化服务: $SERVICE_PATH $SERVICE_ARGS"
    
    # 切换到构建目录
    cd "$BUILD_DIR" || {
        log "错误: 无法切换到构建目录: $BUILD_DIR"
        return 1
    }
    
    # 在后台运行服务并捕获PID
    $SERVICE_PATH $SERVICE_ARGS &
    SERVICE_PID=$!
    
    # 检查服务是否成功启动
    sleep 2
    if ! kill -0 $SERVICE_PID 2>/dev/null; then
        log "错误: 持久化服务启动失败"
        SERVICE_PID=""
        return 1
    fi
    
    log "持久化服务已启动，PID: $SERVICE_PID"
    return 0
}

# 主函数
main() {
    # 处理help参数
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        show_usage
        exit 0
    fi
    
    # 设置信号处理
    trap cleanup SIGINT SIGTERM
    
    # 检查依赖
    check_dependencies
    
    log "======================================="
    log "持久化服务守护脚本启动"
    log "服务路径: $SERVICE_PATH"
    log "服务参数: $SERVICE_ARGS"
    log "工作目录: $BUILD_DIR"
    log "日志文件: $LOG_FILE"
    log "======================================="
    
    # 初始化计数器
    local restart_count=0
    local rapid_restart_count=0
    local last_rapid_restart_time=0
    
    # 主循环
    while $RUNNING; do
        # 启动服务
        local start_time=$(date +%s)
        
        if ! start_service; then
            log "服务启动失败，等待${RESTART_DELAY}秒后重试..."
            sleep $RESTART_DELAY
            continue
        fi
        
        # 等待服务退出
        wait_for_service
        local exit_code=$?
        local end_time=$(date +%s)
        local runtime=$((end_time - start_time))
        
        # 如果脚本被要求停止，退出循环
        if ! $RUNNING; then
            break
        fi
        
        # 检查退出状态
        if [ $exit_code -eq 0 ]; then
            log "持久化服务正常退出，退出码: $exit_code，运行时间: ${runtime}秒"
            log "服务正常退出，守护脚本也将退出"
            break  # 正常退出，不需要重启
        else
            log "持久化服务异常退出，退出码: $exit_code，运行时间: ${runtime}秒"
            
            # 检测是否是快速崩溃（运行时间少于10秒）
            if [ $runtime -lt 10 ]; then
                local current_time=$(date +%s)
                
                # 检查是否在快速重启窗口内
                if [ $((current_time - last_rapid_restart_time)) -lt $RAPID_RESTART_WINDOW ]; then
                    rapid_restart_count=$((rapid_restart_count + 1))
                else
                    rapid_restart_count=1
                    last_rapid_restart_time=$current_time
                fi
                
                log "警告: 服务快速退出 (运行时间: ${runtime}秒，快速重启计数: $rapid_restart_count)"
                
                # 如果短时间内多次快速崩溃，增加等待时间
                if [ $rapid_restart_count -ge $MAX_RAPID_RESTARTS ]; then
                    log "检测到连续${rapid_restart_count}次快速崩溃，等待${BACKOFF_DELAY}秒后再尝试..."
                    sleep $BACKOFF_DELAY
                    rapid_restart_count=0
                else
                    log "等待${RESTART_DELAY}秒后重启服务..."
                    sleep $RESTART_DELAY
                fi
            else
                # 如果运行时间较长，重置快速重启计数
                rapid_restart_count=0
                log "等待${RESTART_DELAY}秒后重启服务..."
                sleep $RESTART_DELAY
            fi
            
            restart_count=$((restart_count + 1))
            log "准备重启服务 (总重启次数: $restart_count)"
        fi
        
        # 清理PID
        SERVICE_PID=""
    done
    
    log "守护脚本正常退出"
    return 0
}

# 脚本入口点
main "$@"