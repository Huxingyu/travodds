#!/bin/bash

# 持久化服务功能测试脚本

cd /home/wangshuo/Agiros-DDS/travodds/example/persistence/build

echo "=== 启动持久化服务测试 ==="

# 1. 清理旧的数据库
echo "清理旧数据..."
rm -f enhanced_persistence.db persistence.db

# 2. 启动持久化服务
echo "启动持久化服务..."
./persistence_service_app &
PERSISTENCE_PID=$!
sleep 3

# 3. 发布一些数据
echo "发布测试数据..."
echo "pub" | timeout 8s ./persistent_hello_world &
PUB_PID=$!
sleep 10

# 4. 检查数据库是否有数据
echo "检查数据库..."
if [ -f "enhanced_persistence.db" ]; then
    echo "数据库文件已创建"
    sqlite3 enhanced_persistence.db ".tables" > tables.txt
    echo "数据库表:"
    cat tables.txt
    
    # 检查数据记录
    if sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM persistent_data;" 2>/dev/null; then
        RECORD_COUNT=$(sqlite3 enhanced_persistence.db "SELECT COUNT(*) FROM persistent_data;" 2>/dev/null || echo "0")
        echo "数据库记录数: $RECORD_COUNT"
        
        if [ "$RECORD_COUNT" -gt 0 ]; then
            echo "显示前5条记录:"
            sqlite3 enhanced_persistence.db "SELECT data_id, topic_id, instance_id, data_size, durability_kind FROM persistent_data LIMIT 5;" 2>/dev/null || echo "查询失败"
        fi
    else
        echo "persistent_data表不存在或查询失败"
    fi
else
    echo "数据库文件未创建"
fi

# 5. 启动订阅者测试历史数据
echo "启动订阅者测试历史数据..."
echo "sub" | timeout 8s ./persistent_hello_world &
SUB_PID=$!
sleep 10

# 6. 清理进程
echo "清理测试进程..."
kill $PERSISTENCE_PID $PUB_PID $SUB_PID 2>/dev/null
wait 2>/dev/null

echo "=== 测试完成 ==="
