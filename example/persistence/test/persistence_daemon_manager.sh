#!/bin/bash
# 持久化服务守护程序使用示例

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WATCHDOG_SCRIPT="$SCRIPT_DIR/persistence_service_watchdog.sh"
PID_FILE="$SCRIPT_DIR/watchdog.pid"
BUILD_DIR="$(cd "$SCRIPT_DIR/../build" && pwd)"
LOG_FILE="$BUILD_DIR/persistence_watchdog.log"

show_usage() {
    echo "持久化服务守护程序管理脚本"
    echo ""
    echo "用法: $0 {start|stop|restart|status|logs|gui}"
    echo ""
    echo "命令:"
    echo "  start     启动守护进程（默认参数）"
    echo "  stop      停止守护进程"
    echo "  restart   重启守护进程"
    echo "  status    查看运行状态"
    echo "  logs      查看日志（实时）"
    echo "  gui       启动GUI模式守护进程"
    echo ""
}

start_daemon() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "守护进程已在运行 (PID: $pid)"
            return 1
        else
            rm -f "$PID_FILE"
        fi
    fi
    
    echo "启动持久化服务守护进程..."
    nohup "$WATCHDOG_SCRIPT" 0 enhanced_persistence.db > /dev/null 2>&1 &
    local pid=$!
    echo $pid > "$PID_FILE"
    echo "守护进程已启动 (PID: $pid)"
    echo "日志文件: $LOG_FILE"
}

start_gui_daemon() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "守护进程已在运行 (PID: $pid)"
            return 1
        else
            rm -f "$PID_FILE"
        fi
    fi
    
    echo "启动持久化服务守护进程 (GUI模式)..."
    nohup "$WATCHDOG_SCRIPT" --gui > /dev/null 2>&1 &
    local pid=$!
    echo $pid > "$PID_FILE"
    echo "守护进程已启动 (PID: $pid, GUI模式)"
    echo "日志文件: $LOG_FILE"
}

stop_daemon() {
    if [ ! -f "$PID_FILE" ]; then
        echo "守护进程未运行"
        return 1
    fi
    
    local pid=$(cat "$PID_FILE")
    if ! kill -0 "$pid" 2>/dev/null; then
        echo "守护进程未运行 (PID文件过期)"
        rm -f "$PID_FILE"
        return 1
    fi
    
    echo "停止守护进程 (PID: $pid)..."
    kill -TERM "$pid" 2>/dev/null
    
    # 等待进程退出
    local count=0
    while [ $count -lt 10 ] && kill -0 "$pid" 2>/dev/null; do
        sleep 1
        count=$((count + 1))
    done
    
    if kill -0 "$pid" 2>/dev/null; then
        echo "强制终止守护进程..."
        kill -KILL "$pid" 2>/dev/null
    fi
    
    rm -f "$PID_FILE"
    echo "守护进程已停止"
}

check_status() {
    if [ ! -f "$PID_FILE" ]; then
        echo "守护进程未运行"
        return 1
    fi
    
    local pid=$(cat "$PID_FILE")
    if kill -0 "$pid" 2>/dev/null; then
        echo "守护进程正在运行 (PID: $pid)"
        
        # 检查持久化服务进程
        local service_pids=$(pgrep -f persistence_service_app)
        if [ -n "$service_pids" ]; then
            echo "持久化服务进程: $service_pids"
        else
            echo "持久化服务当前未运行"
        fi
        return 0
    else
        echo "守护进程未运行 (PID文件过期)"
        rm -f "$PID_FILE"
        return 1
    fi
}

show_logs() {
    if [ -f "$LOG_FILE" ]; then
        echo "显示最新日志 (按Ctrl+C退出):"
        echo "======================================="
        tail -f "$LOG_FILE"
    else
        echo "日志文件不存在: $LOG_FILE"
    fi
}

case "$1" in
    start)
        start_daemon
        ;;
    stop)
        stop_daemon
        ;;
    restart)
        stop_daemon
        sleep 2
        start_daemon
        ;;
    status)
        check_status
        ;;
    logs)
        show_logs
        ;;
    gui)
        start_gui_daemon
        ;;
    *)
        show_usage
        exit 1
        ;;
esac