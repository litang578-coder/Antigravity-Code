<template>
  <view class="status-bar">
    <view class="status-bar__badge">
      <view class="status-bar__dot"></view>
      <text class="status-bar__text">系统运行中</text>
    </view>
    <text class="status-bar__hint">实时设备监测面板</text>
  </view>
</template>

<script>
export default {
  name: 'DashboardStatus'
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.status-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: $space-lg;
  padding: $space-lg $space-xl;
  border: 1rpx solid rgba(143, 205, 255, 0.16);
  border-radius: 28rpx;
  background: $color-panel-bg;
  backdrop-filter: blur(20rpx);
  box-shadow: 0 0 30rpx rgba(0, 112, 255, 0.1);
}

.status-bar__badge {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.status-bar__dot {
  width: 18rpx;
  height: 18rpx;
  border-radius: 50%;
  background: $color-accent-success;
  box-shadow: 0 0 10rpx rgba(57, 255, 136, 0.8);
  animation: breathing 1.8s ease-in-out infinite;
}

.status-bar__text {
  font-size: 30rpx;
  font-weight: 600;
  color: #ebf7ff;
}

.status-bar__hint {
  font-size: 24rpx;
  color: rgba(201, 222, 255, 0.68);
}

@keyframes breathing {
  0%,
  100% {
    transform: scale(0.95);
    opacity: 0.55;
    box-shadow: 0 0 8rpx rgba(57, 255, 136, 0.55);
  }

  50% {
    transform: scale(1.15);
    opacity: 1;
    box-shadow: 0 0 22rpx rgba(57, 255, 136, 0.95);
  }
}

@media screen and (max-width: 480px) {
  .status-bar {
    gap: 12rpx;
    padding: 20rpx 22rpx;
    border-radius: 24rpx;
  }

  .status-bar__text {
    font-size: 26rpx;
  }

  .status-bar__hint {
    font-size: 21rpx;
    text-align: right;
  }
}

@media screen and (max-width: 360px) {
  .status-bar {
    flex-direction: column;
    align-items: flex-start;
  }

  .status-bar__hint {
    text-align: left;
  }
}
</style>
