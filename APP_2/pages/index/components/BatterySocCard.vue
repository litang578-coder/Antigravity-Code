<template>
  <view class="data-card data-card--battery" :class="{ 'data-card--placeholder': !isSocReady }">
    <view class="battery-card__header">
      <text class="data-card__label">电量 SOC</text>
      <text class="battery-card__channel" :class="channelClass">{{ channelText }}</text>
    </view>

    <view class="battery-card__content">
      <view class="battery-card__info">
        <view class="data-card__value-group">
          <text v-if="isSocReady" class="data-card__value" :class="socColorClass">{{ socSafe }}</text>
          <text v-else class="data-card__value data-card__value--placeholder">连接中...</text>
          <text v-if="isSocReady" class="data-card__unit">%</text>
        </view>
        <text class="battery-card__status">{{ isSocReady ? socStatusText : 'Waiting For Data' }}</text>
      </view>

      <view class="battery-icon" :class="batteryIconClass">
        <view class="battery-icon__body">
          <view
            class="battery-icon__fill"
            :class="socFillClass"
            :style="{ width: `${socSafe}%` }"
          ></view>
          <view v-if="showChargingBolt" class="battery-icon__bolt"></view>
        </view>
        <view class="battery-icon__cap"></view>
      </view>
    </view>
  </view>
</template>

<script>
export default {
  name: 'BatterySocCard',
  props: {
    ready: {
      type: Boolean,
      default: false
    },
    soc: {
      type: [Number, String],
      default: null
    },
    relayBat: {
      type: Boolean,
      default: false
    },
    isCharging: {
      type: Boolean,
      default: false
    },
    isChargingUp: {
      type: Boolean,
      default: false
    }
  },
  computed: {
    isSocReady() {
      return this.ready && this.soc !== null && !Number.isNaN(Number(this.soc))
    },
    socSafe() {
      const value = Number(this.soc)
      if (Number.isNaN(value)) return 0
      return Math.min(100, Math.max(0, Math.round(value)))
    },
    socLevel() {
      if (this.socSafe > 50) return 'high'
      if (this.socSafe >= 20) return 'medium'
      return 'low'
    },
    channelText() {
      if (!this.ready) return '等待数据'
      if (!this.isCharging) return '充电未开启'
      return this.relayBat ? '电池 2 充电中' : '电池 1 充电中'
    },
    channelClass() {
      return {
        'battery-card__channel--active': this.ready && this.isCharging,
        'battery-card__channel--idle': this.ready && !this.isCharging
      }
    },
    socStatusText() {
      if (this.isCharging) return this.relayBat ? '电池 2 充电中' : '电池 1 充电中'
      if (this.socLevel === 'high') return '电量充足'
      if (this.socLevel === 'medium') return '警告关注'
      return '低电量预警'
    },
    socColorClass() {
      return `data-card__value--battery-${this.socLevel}`
    },
    socFillClass() {
      return `battery-icon__fill--${this.socLevel}`
    },
    batteryIconClass() {
      return {
        'battery-icon--low': this.isSocReady && this.socLevel === 'low'
      }
    },
    showChargingBolt() {
      return this.isSocReady && this.isCharging
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.data-card {
  @include data-card-shell;
}

.data-card--placeholder {
  .data-card__label,
  .data-card__unit,
  .battery-card__status,
  .battery-card__channel {
    color: $color-text-placeholder;
  }
}

.battery-card__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: $space-md;
}

.data-card__label {
  display: block;
  font-size: 26rpx;
  letter-spacing: 4rpx;
  color: rgba(183, 211, 247, 0.72);
}

.battery-card__channel {
  flex-shrink: 0;
  max-width: 280rpx;
  padding: 8rpx 16rpx;
  border-radius: 999rpx;
  border: 1rpx solid rgba(149, 170, 196, 0.24);
  background: rgba(19, 27, 43, 0.58);
  color: rgba(191, 216, 255, 0.72);
  font-size: 22rpx;
  line-height: 1.3;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.battery-card__channel--active {
  border-color: rgba(0, 229, 255, 0.44);
  background: rgba(0, 229, 255, 0.12);
  color: rgba(214, 245, 255, 0.94);
  box-shadow: 0 0 18rpx rgba(0, 229, 255, 0.16);
}

.battery-card__channel--idle {
  border-color: rgba(255, 213, 118, 0.28);
  background: rgba(255, 194, 61, 0.1);
  color: rgba(255, 227, 166, 0.82);
}

.data-card__value-group {
  display: flex;
  align-items: flex-end;
  gap: 12rpx;
  margin-top: $space-md;
}

.data-card__value {
  font-size: 78rpx;
  line-height: 1;
  font-weight: 700;
  text-shadow: 0 0 18rpx currentColor;
  transition: color 0.5s ease;
}

.data-card__value--battery-high {
  color: $color-battery-high;
}

.data-card__value--battery-medium {
  color: $color-battery-medium;
}

.data-card__value--battery-low {
  color: $color-battery-low;
}

.data-card__value--placeholder {
  font-size: 44rpx;
  line-height: 1.2;
  color: $color-text-placeholder;
  text-shadow: none;
}

.data-card__unit {
  font-size: 30rpx;
  line-height: 1.2;
  color: rgba(226, 238, 255, 0.78);
  padding-bottom: 10rpx;
}

.data-card--battery {
  overflow: hidden;
}

.battery-card__content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: $space-md;
  margin-top: $space-md;
}

.battery-card__info {
  flex: 1;
  min-width: 0;
}

.battery-card__status {
  display: block;
  margin-top: 14rpx;
  font-size: 22rpx;
  color: $color-text-soft;
  letter-spacing: 1rpx;
}

.battery-icon {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.battery-icon__body {
  position: relative;
  width: 132rpx;
  height: 64rpx;
  padding: 8rpx;
  box-sizing: border-box;
  border-radius: 18rpx;
  border: 2rpx solid rgba(210, 233, 255, 0.7);
  background: rgba(7, 18, 34, 0.45);
  backdrop-filter: blur(12rpx);
  box-shadow:
    inset 0 0 18rpx rgba(255, 255, 255, 0.06),
    0 0 24rpx rgba(71, 184, 255, 0.12);
  overflow: hidden;
  transition: all 0.5s ease-out;
}

.battery-icon__cap {
  width: 10rpx;
  height: 24rpx;
  border-radius: 0 8rpx 8rpx 0;
  background: rgba(210, 233, 255, 0.72);
  box-shadow: 0 0 12rpx rgba(105, 199, 255, 0.24);
  transition: all 0.5s ease-out;
}

.battery-icon__fill {
  height: 100%;
  border-radius: 10rpx;
  transition: all 0.5s ease-out;
}

.battery-icon__fill--high {
  background: linear-gradient(90deg, rgba(16, 185, 129, 0.72) 0%, #10b981 100%);
  box-shadow: 0 0 18rpx rgba(16, 185, 129, 0.45);
}

.battery-icon__fill--medium {
  background: linear-gradient(90deg, rgba(245, 158, 11, 0.72) 0%, #f59e0b 100%);
  box-shadow: 0 0 18rpx rgba(245, 158, 11, 0.45);
}

.battery-icon__fill--low {
  background: linear-gradient(90deg, rgba(239, 68, 68, 0.72) 0%, #ef4444 100%);
  box-shadow: 0 0 18rpx rgba(239, 68, 68, 0.55);
}

.battery-icon--low .battery-icon__body {
  border-color: rgba(239, 68, 68, 0.88);
  box-shadow:
    inset 0 0 18rpx rgba(255, 255, 255, 0.04),
    0 0 24rpx rgba(239, 68, 68, 0.18);
  animation: battery-danger-breathing 1.8s ease-in-out infinite;
}

.battery-icon--low .battery-icon__cap {
  background: rgba(239, 68, 68, 0.88);
  box-shadow: 0 0 12rpx rgba(239, 68, 68, 0.32);
}

.battery-icon__bolt {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 24rpx;
  height: 34rpx;
  transform: translate(-50%, -50%);
  background: linear-gradient(180deg, #fde68a 0%, #facc15 100%);
  clip-path: polygon(45% 0, 100% 0, 64% 42%, 100% 42%, 32% 100%, 44% 58%, 0 58%);
  filter: drop-shadow(0 0 10rpx rgba(250, 204, 21, 0.6));
  animation: battery-bolt-pulse 1s ease-in-out infinite;
}

@keyframes battery-danger-breathing {
  0%,
  100% {
    box-shadow:
      inset 0 0 18rpx rgba(255, 255, 255, 0.04),
      0 0 16rpx rgba(239, 68, 68, 0.14);
    transform: scale(1);
  }

  50% {
    box-shadow:
      inset 0 0 18rpx rgba(255, 255, 255, 0.05),
      0 0 26rpx rgba(239, 68, 68, 0.32);
    transform: scale(1.02);
  }
}

@keyframes battery-bolt-pulse {
  0%,
  100% {
    opacity: 0.72;
    transform: translate(-50%, -50%) scale(0.96);
  }

  50% {
    opacity: 1;
    transform: translate(-50%, -50%) scale(1.06);
  }
}

@media screen and (max-width: 480px) {
  .data-card {
    min-height: 0;
    padding: 26rpx 24rpx;
  }

  .data-card__value {
    font-size: 70rpx;
  }

  .battery-card__content {
    align-items: flex-end;
  }

  .battery-card__channel {
    max-width: 220rpx;
    font-size: 20rpx;
  }

  .battery-icon__body {
    width: 118rpx;
    height: 58rpx;
  }

}

@media screen and (max-width: 360px) {
  .battery-card__header {
    align-items: flex-start;
    flex-direction: column;
  }

  .battery-card__channel {
    max-width: 100%;
  }

  .battery-card__content {
    flex-direction: column;
    align-items: flex-start;
  }

  .battery-icon {
    align-self: flex-end;
  }
}
</style>
