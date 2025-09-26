#!/bin/bash

echo "=== TravoDDS 持久化策略完整测试脚本 ==="

# 设置工作目录
WORK_DIR="/home/wangshuo/Agiros-DDS/travodds/example/persistence/build"
cd "$WORK_DIR"

# 清理之前的文件
echo "清理之前的测试文件..."
rm -f *.db* *.log *.ddslog
killall -9 persistence_service_app 2>/dev/null
sleep 2

echo "1. 启动持久化服务..."
nohup ./persistence_service_app 0 enhanced_persistence.db > persistence_service.log 2>&1 &
PERSISTENCE_PID=$!
echo "持久化服务 PID: $PERSISTENCE_PID"
sleep 8

# 检查持久化服务是否正常启动
if ! kill -0 $PERSISTENCE_PID 2>/dev/null; then
    echo "❌ 错误: 持久化服务启动失败"
    cat persistence_service.log
    exit 1
fi
echo "✅ 持久化服务启动成功"

echo ""
echo "2. 测试四种持久化策略的发布者..."

strategies=("volatile:VOLATILE" "transient_local:TRANSIENT_LOCAL" "transient:TRANSIENT" "persistent:PERSISTENT")

for strategy_pair in "${strategies[@]}"; do
    IFS=':' read -r strategy_name strategy_display <<< "$strategy_pair"
    
    echo "2.${#strategies[@]} 测试 $strategy_display 策略..."
    
    # 运行发布者并等待完成 (使用stdbuf禁用缓冲)
    stdbuf -oL -eL timeout 15s ./persistent_hello_world pub "$strategy_name" > "${strategy_name}_pub.log" 2>&1
    exit_code=$?
    
    # 等待输出写入完成
    sync
    sleep 1
    
    if [ $exit_code -eq 0 ] || [ $exit_code -eq 124 ]; then
        # 检查日志文件是否有内容
        if [ -s "${strategy_name}_pub.log" ]; then
            success_count=$(grep -c "发送成功:" "${strategy_name}_pub.log" 2>/dev/null || echo "0")
            total_lines=$(wc -l < "${strategy_name}_pub.log" 2>/dev/null || echo "0")
            echo "    ✅ $strategy_display 发布者完成，发送了 $success_count 条消息 (日志 $total_lines 行)"
        else
            echo "    ⚠️ $strategy_display 发布者完成，但无日志输出 (退出码: $exit_code)"
        fi
    else
        echo "    ❌ $strategy_display 发布者运行失败 (退出码: $exit_code)"
        echo "    日志内容:"
        head -5 "${strategy_name}_pub.log" | sed 's/^/      /'
    fi
    
    # 等待数据处理
    sleep 2
done

echo ""
echo "3. 等待所有数据处理完成..."
sleep 5

echo ""
echo "4. 检查数据库状态..."
if [ -f enhanced_persistence.db ]; then
    echo "✅ 数据库文件存在"
    
    echo "4.1 检查注册的主题:"
    sqlite3 enhanced_persistence.db "SELECT topic_name, type_name, total_samples FROM topics;" 2>/dev/null | while read line; do
        echo "    $line"
    done
    
    echo "4.2 检查样本总数:"
    sample_count=$(sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM samples;" 2>/dev/null || echo "0")
    echo "    数据库中的样本总数: $sample_count"
    
    if [ "$sample_count" -gt 0 ]; then
        echo "4.3 按主题统计数据:"
        sqlite3 enhanced_persistence.db "SELECT t.topic_name, COUNT(s.id) as count FROM topics t LEFT JOIN samples s ON t.id = s.topic_id GROUP BY t.topic_name;" 2>/dev/null | while read line; do
            echo "    $line"
        done
        
        echo "4.4 最新的几条样本数据:"
        sqlite3 enhanced_persistence.db "SELECT t.topic_name, s.durability_kind, datetime(s.reception_timestamp/1000000, 'unixepoch') FROM samples s JOIN topics t ON s.topic_id = t.id ORDER BY s.reception_timestamp DESC LIMIT 3;" 2>/dev/null | while read line; do
            echo "    $line"
        done
    fi
else
    echo "❌ 数据库文件不存在"
fi

echo ""
echo "5. 测试历史数据恢复..."

echo "5.1 测试 PERSISTENT 策略订阅者 (应该接收历史数据):"
stdbuf -oL -eL timeout 12s ./persistent_hello_world sub persistent > persistent_sub.log 2>&1 &
sleep 13

# 等待输出写入完成
sync
sleep 1

if [ -s persistent_sub.log ]; then
    received_count=$(grep -c "Received data" persistent_sub.log 2>/dev/null || echo "0")
    total_lines=$(wc -l < persistent_sub.log 2>/dev/null || echo "0")
    echo "    接收到 $received_count 条历史消息 (日志 $total_lines 行)"

    if [ "$received_count" -gt 0 ]; then
        echo "    示例消息:"
        grep "Received data" persistent_sub.log | head -2 | sed 's/^/      /'
    fi
else
    echo "    ⚠️ PERSISTENT 订阅者无日志输出"
fi

echo "5.2 测试 TRANSIENT 策略订阅者:"
stdbuf -oL -eL timeout 8s ./persistent_hello_world sub transient > transient_sub.log 2>&1 &
sleep 9

# 等待输出写入完成
sync
sleep 1

if [ -s transient_sub.log ]; then
    received_count=$(grep -c "Received data" transient_sub.log 2>/dev/null || echo "0")
    total_lines=$(wc -l < transient_sub.log 2>/dev/null || echo "0")
    echo "    接收到 $received_count 条历史消息 (日志 $total_lines 行)"
else
    echo "    ⚠️ TRANSIENT 订阅者无日志输出"
fi

echo ""
echo "6. 测试 GUI 模式 (可选)..."
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    echo "检测到图形环境，测试 GUI 启动..."
    timeout 5s ./persistence_service_app --gui > gui_test.log 2>&1 &
    GUI_PID=$!
    sleep 6
    
    if kill -0 $GUI_PID 2>/dev/null; then
        echo "✅ GUI 模式启动成功"
        kill $GUI_PID 2>/dev/null
        sleep 1
    else
        echo "⚠️ GUI 模式可能有问题 (退出码: $(wait $GUI_PID; echo $?))"
        echo "GUI 日志:"
        head -10 gui_test.log | sed 's/^/    /'
    fi
else
    echo "⚠️ 未检测到图形环境，跳过 GUI 测试"
fi

echo ""
echo "7. 最终状态检查..."

echo "7.1 持久化服务状态:"
if kill -0 $PERSISTENCE_PID 2>/dev/null; then
    echo "    ✅ 持久化服务正在运行"
    
    # 显示最新统计
    echo "    最新统计信息:"
    tail -30 persistence_service.log | grep -A 10 "Persistence Statistics" | tail -10 | sed 's/^/      /'
else
    echo "    ❌ 持久化服务已停止"
fi

echo "7.2 最终数据库状态:"
if [ -f enhanced_persistence.db ]; then
    final_sample_count=$(sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM samples;" 2>/dev/null || echo "0")
    topic_count=$(sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM topics;" 2>/dev/null || echo "0")
    echo "    数据库样本总数: $final_sample_count"
    echo "    注册的主题数量: $topic_count"
    
    db_size=$(ls -lh enhanced_persistence.db | awk '{print $5}')
    echo "    数据库文件大小: $db_size"
    
    # 显示主题详情
    if [ $topic_count -gt 0 ]; then
        echo "    主题详情:"
        sqlite3 enhanced_persistence.db "SELECT name, durability_kind, sample_count FROM topics ORDER BY sample_count DESC LIMIT 5;" 2>/dev/null | sed 's/^/      /' || echo "      查询失败"
    fi
fi

echo ""
echo "8. 清理..."
if kill -0 $PERSISTENCE_PID 2>/dev/null; then
    echo "停止持久化服务..."
    kill $PERSISTENCE_PID
    sleep 3
    if kill -0 $PERSISTENCE_PID 2>/dev/null; then
        kill -9 $PERSISTENCE_PID 2>/dev/null
    fi
fi

echo ""
echo "=== 测试完成 ==="
echo "📊 测试总结:"
echo "  持久化服务: $(kill -0 $PERSISTENCE_PID 2>/dev/null && echo '运行正常' || echo '已停止')"
echo "  数据库文件: $([ -f enhanced_persistence.db ] && echo '已创建' || echo '未创建')"
echo "  样本数据: $(sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM samples;" 2>/dev/null || echo "0") 条"
echo ""
echo "📁 生成的文件:"
ls -la *.log *.db* 2>/dev/null | sed 's/^/  /'
