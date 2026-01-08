#include "rtps/builtin/dpr/dynamicperformanceregulator.h"
#include "common/time/ddstimer.h"
#include "common/log/logger.h"
#include "rtps/flowcontrol/flowratelimitedmanager.h"
#include <cmath>

#ifdef __linux__
#include <fstream>
#include <sstream>
#include <regex>
#endif

USING_TRAVODDS_NAMESPACE;

DynamicPerformanceRegulator::DynamicPerformanceRegulator(const DynamicAdjustQosPolicy& policy, FlowRateLimitedManager* flowRateLimitedManager)
	: count_(0), flowRateLimitedManager_(flowRateLimitedManager), config_(policy)
{
	// 设置调整间隔
	timer_ = TimerManager::CreateTimer(
		reinterpret_cast<const Time_t&>(policy.adjustment_interval),
		[this]() {
			this->UpdateNetDevStats();
		},
		"dpr"
	);

}

DynamicPerformanceRegulator::~DynamicPerformanceRegulator()
{
	if (timer_) {
		timer_->Exit();
	}
}

void DynamicPerformanceRegulator::RegisterInterface(std::string face)
{
	std::lock_guard<std::mutex> lock(statsMtx_);
	auto faceIt = facestats_.find(face);
	if (faceIt == facestats_.end()) {
		NetworkStats stats;
#ifdef __linux__
		std::string speedPath = "/sys/class/net/" + face + "/speed";
		std::ifstream speedFile(speedPath);
		
		if (face != "lo" && !IsWirelessInterface(face) && speedFile.is_open()) {
			speedFile >> stats.linkSpeed;
		}
#endif
		facestats_.insert({ face, stats });
	}
}

void DynamicPerformanceRegulator::UnregisterInterface(std::string face)
{
	std::lock_guard<std::mutex> lock(statsMtx_);
	facestats_.erase(face);
}

void DynamicPerformanceRegulator::UpdateNetDevStats()
{
	DynamicPerformanceRegulator::Score score;
#ifdef __linux__
	std::ifstream file("/proc/net/dev");
	std::string line;

	// 跳过前两行标题
	std::getline(file, line);
	std::getline(file, line);

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string face;

		iss >> face;
		if (face.back() == ':') {
			face.pop_back();
		}
		std::lock_guard<std::mutex> lock(statsMtx_);
		auto faceIt = facestats_.find(face);
		if (faceIt == facestats_.end()) {
			continue;
		}
		NetworkStats stats;
		stats.noInitStats = true;
		// 读取接收统计
		uint64_t temp;
		iss >> stats.rxBytes >> stats.rxPackets >> stats.rxErrs >> stats.rxDrop
			>> stats.rxFifo >> stats.rxFrame >> temp >> stats.rxMulticast
			>> stats.txBytes >> stats.txPackets >> stats.txErrs >> stats.txDrop
			>> stats.txFifo >> stats.txColls >> stats.txCarrier >> temp;

		auto rscore = CalculateMetricsScore(stats, faceIt->second);
		if (rscore.value > score.value) {
			score = rscore;
		}

		stats.linkSpeed = faceIt->second.linkSpeed;
		faceIt->second = stats;
	}
#endif

	Adjustment(score);
}

DynamicPerformanceRegulator::Score  DynamicPerformanceRegulator::CalculateMetricsScore(const NetworkStats& currentStats, const NetworkStats& lastStats) const
{
	Score score;
	double seconds = config_.policy.adjustment_interval .sec + config_.policy.adjustment_interval.nanosec * 1.0e-9;

	if (lastStats.noInitStats) {
		EnhancedNetworkMetrics metrics;
		metrics.linkSpeed = lastStats.linkSpeed;
		// 计算基础速率
		uint64_t rxBytesDiff = currentStats.rxBytes - lastStats.rxBytes;
		uint64_t txBytesDiff = currentStats.txBytes - lastStats.txBytes;
		uint64_t rxPacketsDiff = currentStats.rxPackets - lastStats.rxPackets;
		uint64_t txPacketsDiff = currentStats.txPackets - lastStats.txPackets;
		metrics.rxRatembps = (rxBytesDiff * 8.0) / (seconds * 1000000.0);
		metrics.txRatembps = (txBytesDiff * 8.0) / (seconds * 1000000.0);
		score.txmbps = metrics.txRatembps;

		if (rxPacketsDiff > 0) {
			// 接收质量指标
			metrics.rxPacketLossRate = (currentStats.rxDrop - lastStats.rxDrop) * 100.0 / rxPacketsDiff;
			metrics.rxErrorRate = (currentStats.rxErrs - lastStats.rxErrs) * 100.0 / rxPacketsDiff;
			metrics.rxFifoErrorRate = (currentStats.rxFifo - lastStats.rxFifo) * 100.0 / rxPacketsDiff;
			metrics.rxFrameErrorRate = (currentStats.rxFrame - lastStats.rxFrame) * 100.0 / rxPacketsDiff;

			// 多播比例
			uint64_t rxMulticast_diff = currentStats.rxMulticast - lastStats.rxMulticast;
			metrics.multicastRatio = rxMulticast_diff * 100.0 / rxPacketsDiff;
		}

		if (txPacketsDiff > 0) {
			// 发送质量指标
			metrics.txErrorRate = (currentStats.txErrs - lastStats.txErrs) * 100.0 / txPacketsDiff;
			metrics.txDropRate = (currentStats.txDrop - lastStats.txDrop) * 100.0 / txPacketsDiff;
			metrics.txFifoErrorRate = (currentStats.txFifo - lastStats.txFifo) * 100.0 / txPacketsDiff;
			metrics.txCollisionRate = (currentStats.txColls - lastStats.txColls) * 100.0 / txPacketsDiff;
			metrics.txCarrierErrorRate = (currentStats.txCarrier - lastStats.txCarrier) * 100.0 / txPacketsDiff;
		}
		// 计算得分
		// 计分规则：输出区间 [0,100]，值越大表示风险越高，需要更强的调节力度
		{
			auto clamp = [](const double& value, const double& low, const double& high) {
				return (value < low) ? low : (value > high) ? high : value;
				};
			auto clampNorm = [clamp](double rate) { return clamp(rate / 100.0, 0.0, 1.0); };

			double weightBw = 0.20;
			double weightRx = 0.40;
			double weightTx = 0.40;

			// 1) 带宽利用率惩罚：采用观测到的最高方向占用
			double bwPenalty = 0.0;
			if (metrics.linkSpeed > 0.0) {
				score.txrateratio = metrics.txRatembps / metrics.linkSpeed;
				double peakMbps = std::max(metrics.rxRatembps, metrics.txRatembps);
				double util = peakMbps / metrics.linkSpeed;

				// 仅当 util >= 0.9 时参与评分，避免低/中等利用率导致不必要的调节
				if (util >= 0.9 && util <= 1.0) {
					// 90%-100% 线性映射到 0.5-1.0
					bwPenalty = 0.5 + (util - 0.9) * 5.0; // (1.0-0.5)/(1.0-0.9) = 5.0
					bwPenalty = clamp(bwPenalty, 0.5, 1.0);
				} else if (util > 1.0) {
					bwPenalty = 1.0; // 超100%利用率，最大惩罚
				} else {
					bwPenalty = 0.0; // 低于90%，无惩罚
				}
			}

			// 2) 接收质量惩罚：丢包、错误、FIFO、帧错
			double rxPenalty = 0.0;
			{
				// 归一化并限制范围
				double rxLoss = clampNorm(metrics.rxPacketLossRate);
				double rxErr = clampNorm(metrics.rxErrorRate);
				double rxFifo = clampNorm(metrics.rxFifoErrorRate);
				double rxFrame = clampNorm(metrics.rxFrameErrorRate);

				// 非线性权重：高丢包率时权重自动增加
				double lossWeight = 0.5;
				double errWeight = 0.25;
				double fifoWeight = 0.15;
				double frameWeight = 0.10;

				// 如果丢包率很高，自动增加其权重
				if (rxLoss > 0.1) { // >10%丢包
					double boost = (rxLoss - 0.1) * 2.0; // 线性提升
					lossWeight = std::min(0.8, 0.5 + boost);
					// 相应降低其他权重
					double totalOther = errWeight + fifoWeight + frameWeight;
					errWeight *= (1.0 - (lossWeight - 0.5)) / totalOther;
					fifoWeight *= (1.0 - (lossWeight - 0.5)) / totalOther;
					frameWeight *= (1.0 - (lossWeight - 0.5)) / totalOther;
				}

				rxPenalty = lossWeight * rxLoss +
					errWeight * rxErr +
					fifoWeight * rxFifo +
					frameWeight * rxFrame;
				rxPenalty = clamp(rxPenalty, 0.0, 1.0);
			}

			// 3) 发送质量惩罚：错误、丢弃、FIFO、碰撞、承载错误
			double txPenalty = 0.0;
			{
				double txErr = clampNorm(metrics.txErrorRate);
				double txDrop = clampNorm(metrics.txDropRate);
				double txFifo = clampNorm(metrics.txFifoErrorRate);
				double txColl = clampNorm(metrics.txCollisionRate);
				double txCarri = clampNorm(metrics.txCarrierErrorRate);

				// 自适应权重：高丢包时增加权重
				double dropWeight = 0.35;
				double errWeight = 0.25;
				double fifoWeight = 0.15;
				double collWeight = 0.15;
				double carriWeight = 0.10;

				if (txDrop > 0.1) { // >10%发送丢包
					double boost = (txDrop - 0.1) * 2.0;
					dropWeight = std::min(0.6, 0.35 + boost);
					// 调整其他权重
					double totalOther = errWeight + fifoWeight + collWeight + carriWeight;
					errWeight *= (1.0 - (dropWeight - 0.35)) / totalOther;
					fifoWeight *= (1.0 - (dropWeight - 0.35)) / totalOther;
					collWeight *= (1.0 - (dropWeight - 0.35)) / totalOther;
					carriWeight *= (1.0 - (dropWeight - 0.35)) / totalOther;
				}

				txPenalty = dropWeight * txDrop +
					errWeight * txErr +
					fifoWeight * txFifo +
					collWeight * txColl +
					carriWeight * txCarri;
				txPenalty = clamp(txPenalty, 0.0, 1.0);
			}

			// 样本量不足时降低敏感度
			double sample_scale = 1.0;
			if (rxPacketsDiff + txPacketsDiff < 100) {
				sample_scale = std::max(0.1, (rxPacketsDiff + txPacketsDiff) / 100.0);
				// 对随机波动较大的指标进行更强的降敏
				rxPenalty = (rxPenalty * 0.3) + (rxPenalty * sample_scale * 0.7);
				txPenalty = (txPenalty * 0.3) + (txPenalty * sample_scale * 0.7);
			}

			// 聚合策略
			double maxPenalty = std::max(std::max(bwPenalty, rxPenalty), txPenalty);

			// noisy-or：多项中等问题可叠加
			double termBw = 1.0 - (weightBw * bwPenalty);
			double termRx = 1.0 - (weightRx * rxPenalty);
			double termTx = 1.0 - (weightTx * txPenalty);

			double noisyOr = 1.0 - (termBw * termRx * termTx);
			noisyOr = clamp(noisyOr, 0.0, 1.0);

			// 动态混合系数：根据最严重程度调整
			double lambdaEmphasizeMax = 0.6; // 基础值

			// 如果存在严重问题（>80%），更倾向于最大值
			if (maxPenalty > 0.8) {
				lambdaEmphasizeMax = 0.8;
			} else if (maxPenalty > 0.5) {
				lambdaEmphasizeMax = 0.7;
			}

			double combined = lambdaEmphasizeMax * maxPenalty +
				(1.0 - lambdaEmphasizeMax) * noisyOr;
			combined = clamp(combined, 0.0, 1.0);

			score.value = combined * 100.0;
		}
	}
	return score;
}
	void DynamicPerformanceRegulator::Adjustment(const Score& score)
	{
		if (score.value <= 0) {
			return;
		}

		// 只有当分数超过配置阈值时触发调整
		if (score.value < config_.policy.score_threshold_factor) {
			// 连续三次低于阈值的80%，流控回调
			const double threshold = config_.policy.score_threshold_factor * 0.8;
			if (config_.bytesPerPeriod != 0 &&  score.value < threshold) {
				count_++;

				if (count_ >= 3) {  
					FlowParameters para;
					PopBack(para);
					flowRateLimitedManager_->ResetPeriod(para);
					if(para.maxBytesPerPeriod == 0) {
						config_.bytesPerPeriod = 625000000;
					} else {
					config_.bytesPerPeriod = para.maxBytesPerPeriod;
					}
					config_.flowcontrolIntervalMs = para.periodMs;

					count_ = 0;
				}
			} else {
				count_ = 0; 
			}
			return;
		}

		// 规范 score 到 [0,100]
		double clampedScore = score.value;
		if (clampedScore < 0.0) clampedScore = 0.0;
		if (clampedScore > 100.0) clampedScore = 100.0;

		// 第一步：计算新的流控周期（ms）
		uint64_t baseInterval = config_.flowcontrolIntervalMs;
		uint64_t maxInterval = config_.flowcontrolMaxIntervalMs; // immutable
		uint64_t minInterval = config_.flowcontrolMinIntervalMs; // immutable

		// 随着 score 增大，网络问题更严重 -> 更频繁的流量控制（周期更小）
       // 映射规则：score=0 -> baseInterval；score=100 -> minInterval（更频繁）
		double interval_val = static_cast<double>(baseInterval);
		if (baseInterval > minInterval) {
			// 将 score 映射为 [baseInterval, minInterval] 之间的线性值（反向）
			interval_val = baseInterval - (static_cast<double>(baseInterval) - static_cast<double>(minInterval)) * (clampedScore / 100.0);
		}

		// 将结果限制到 [minInterval, maxInterval]
		uint64_t newIntervalMs = static_cast<uint64_t>(std::min<double>(static_cast<double>(maxInterval), std::max<double>(static_cast<double>(minInterval), interval_val)));
		
		// 第二步：计算缩放因子并调整周期最大允许字节数

		// 使用观测到的每秒发送速率作为基准（优先），否则使用配置的 bytesPerPeriod 作为基线
		int32_t newMaxBytes = 0;
		double currentTxMbps = score.txmbps;
		if (score.txrateratio > 0) {
			// 将当前速率转换为字节/秒
			double currentBytesPerSec = (currentTxMbps * 1e6) / 8.0;

			// 温和调整策略：逐步降低到目标范围
			double targetReductionRatio = 0.0;
			double scale = 1.0;  // 默认缩放因子

			if (score.txrateratio > 0.9) {
				targetReductionRatio = score.txrateratio - 0.88; // 目标降幅
				scale = 1.0 - targetReductionRatio;
			} else {
				// 低于 0.8，限制最大降幅
				double softMinScale = 0.05;
				scale = 1.0 - (clampedScore / 100.0) * (1.0 - softMinScale);
				// 确保不会过度降低
				scale = std::max(scale, 0.3); // 最低保留30%带宽
			}
			// 计算调整后的字节率
			double adjustedBytesPerSec = currentBytesPerSec * scale;

			// 基于新周期计算每周期字节数
			double bytesPerPeriod = adjustedBytesPerSec * (static_cast<double>(newIntervalMs) / 1000.0);
			newMaxBytes = static_cast<int64_t>(std::max(1.0, std::floor(bytesPerPeriod)));

		}  else if (score.txmbps > 0.0) {
			// 使用观测到的发送速率作为基准
			// observed Mbps -> 转换为 bytes per second，再乘以 scale 得到预期每秒允许字节
			double softMinScale = 0.05;
			double scale = 1.0 - (clampedScore / 100.0) * (1.0 - softMinScale);
			scale = std::max(scale, 0.3); // 限制最大降幅

			double observedBytesPerSec = (score.txmbps * 1e6) / 8.0;
			double allowedBytesPerSec = observedBytesPerSec * scale;
			// 基于新的周期 newIntervalMs 计算每周期字节数
			double bytesPerPeriod = allowedBytesPerSec * (static_cast<double>(newIntervalMs) / 1000.0);
			newMaxBytes = static_cast<int64_t>(std::max(1.0, std::floor(bytesPerPeriod)));
		}  else {
			// 没有观测到发送速率时使用配置的基线
			double softMinScale = 0.05;
			double scale = 1.0 - (clampedScore / 100.0) * (1.0 - softMinScale);
			scale = std::max(scale, 0.3); // 限制最大降幅

			int64_t baseMax = static_cast<int64_t>(config_.bytesPerPeriod);
			// 调整基线值以适应新的周期长度
			double intervalRatio = static_cast<double>(newIntervalMs) / static_cast<double>(baseInterval);
			double adjustedBaseMax = static_cast<double>(baseMax) * intervalRatio;
			newMaxBytes = static_cast<int64_t>(std::max(1.0, std::floor(adjustedBaseMax * scale)));
		}

		// 第三步：强制满足最小流速限制（flowcontrolMinSpeed 单位 Mbps，immutable）
		double minSpeedMbps = static_cast<double>(config_.flowcontrolMinSpeed);
		if (minSpeedMbps > 0.0) {
			// 需要的字节数： minSpeedMbps (bits/s) * interval(s) / 8
			double requiredBytesPerInterval = (minSpeedMbps * 1e6) * (static_cast<double>(newIntervalMs) / 1000.0) / 8.0;
			if (static_cast<double>(newMaxBytes) < requiredBytesPerInterval) {
				newMaxBytes = static_cast<int32_t>(std::ceil(requiredBytesPerInterval));
			}
		}

		FlowParameters para { newMaxBytes, newIntervalMs } ;

		if (!PushBack(para)) {
			return;
		}
		// 记录新策略
		TRAVODDS_LOG(LOG_DEBUG, "DPR: Applying flow control -> max_bytes_per_period=%d, interval_ms=%d, (score=%f)",
			para.maxBytesPerPeriod, para.periodMs, clampedScore);

		flowRateLimitedManager_->ResetPeriod(para);

		config_.bytesPerPeriod = para.maxBytesPerPeriod;
		config_.flowcontrolIntervalMs = para.periodMs;
	}

	bool DynamicPerformanceRegulator::IsWirelessInterface(const std::string& interface) const
	{
#ifdef __linux__
		// 常见的无线网卡命名模式
		static const std::vector<std::regex> wireless_patterns = {
			std::regex("^wlan[0-9]+"),      // wlan0, wlan1等
			std::regex("^wlp[0-9]+s[0-9]+"), // wlp2s0, wlp3s0等
			std::regex("^wifi[0-9]+"),      // wifi0, wifi1等
			std::regex("^wlx[0-9a-f]+"),    // wlx001122334455等
			std::regex("^ath[0-9]+"),       // ath0, ath1等（Atheros芯片）
			std::regex("^ra[0-9]+")         // ra0, ra1等（Ralink芯片）
		};

		// 检查是否匹配无线网卡模式
		for (const auto& pattern : wireless_patterns) {
			if (std::regex_match(interface, pattern)) {
				return true;
			}
		}
#endif
		return false;
	}

	bool DynamicPerformanceRegulator::PushBack(FlowParameters& para)
	{
		bool changed = false;
		if (flowparameters_.empty()) {
			flowparameters_.push_back(para);
			return true;
		}
		// 限制记录容量
		if (flowparameters_.size() >= 10) {
			flowparameters_.pop_front();
		}
		FlowParameters prepara = flowparameters_.front();
		if (std::llabs(static_cast<int64_t>(prepara.maxBytesPerPeriod) - static_cast<int64_t>(para.maxBytesPerPeriod)) > static_cast<int64_t>(prepara.maxBytesPerPeriod / 20 + 1)) {
			changed = true;
		} else {
			para.maxBytesPerPeriod = prepara.maxBytesPerPeriod;
		}
		if (prepara.periodMs != para.periodMs && std::llabs(static_cast<int64_t>(prepara.periodMs) - para.periodMs) > 5) {
			changed = true;
		} else {
			para.periodMs = prepara.periodMs;
		}
		if (changed) {
			flowparameters_.push_back(para);
		}
		return changed;
	}

	void DynamicPerformanceRegulator::PopBack(FlowParameters& para)
	{
		if (!flowparameters_.empty()) {
			para = flowparameters_.back();
			flowparameters_.pop_back();
		}
	}
