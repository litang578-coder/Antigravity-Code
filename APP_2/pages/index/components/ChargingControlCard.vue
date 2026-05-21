<template>
  <view class="control-card">
    <view class="control-panel control-panel--solar">
      <view class="control-panel__header">
        <view class="control-panel__icon control-panel__icon--solar">
          <view class="sun-icon">
            <view class="sun-icon__core"></view>
            <view
              v-for="index in 8"
              :key="index"
              class="sun-icon__ray"
              :class="`sun-icon__ray--${index}`"
            ></view>
          </view>
        </view>
        <view class="control-panel__text">
          <text class="control-panel__title">太阳能充电</text>
          <text
            class="control-panel__status"
            :class="{ 'control-panel__status--active': isCharging }"
          >
            {{ chargingStatusText }}
          </text>
        </view>
      </view>

      <view class="control-panel__footer">
        <text class="control-panel__meta">Solar Charging</text>
        <view
          class="control-switch"
          :class="{
            'control-switch--active': isCharging,
            'control-switch--loading': chargingLoading
          }"
        >
          <switch
            :checked="isCharging"
            :disabled="chargingLoading"
            @change="emitChargingToggle"
          />
          <view v-if="chargingLoading" class="control-switch__loading"></view>
        </view>
      </view>
    </view>

    <view class="control-panel control-panel--battery">
      <view class="control-panel__header">
        <view class="control-panel__icon control-panel__icon--battery">
          <view class="channel-icon">
            <view class="channel-icon__body">
              <view
                class="channel-icon__fill"
                :class="{ 'channel-icon__fill--active': batteryChannelEnabled }"
              ></view>
            </view>
            <view class="channel-icon__cap"></view>
          </view>
        </view>
        <view class="control-panel__text">
          <text class="control-panel__title">电池通道</text>
          <text
            class="control-panel__status"
            :class="{ 'control-panel__status--active': batteryChannelEnabled }"
          >
            {{ batteryChannelStatusText }}
          </text>
        </view>
      </view>

      <view class="control-panel__footer">
        <text class="control-panel__meta">{{ batteryChannelMetaText }}</text>
        <view
          class="control-switch control-switch--battery"
          :class="{
            'control-switch--active': batteryChannelEnabled,
            'control-switch--loading': batteryChannelLoading
          }"
        >
          <switch
            :checked="batteryChannelEnabled"
            :disabled="batteryChannelLoading"
            @change="emitBatteryChannelToggle"
          />
          <view v-if="batteryChannelLoading" class="control-switch__loading"></view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
export default {
  name: 'ChargingControlCard',
  props: {
    isCharging: {
      type: Boolean,
      default: false
    },
    chargingLoading: {
      type: Boolean,
      default: false
    },
    batteryChannelEnabled: {
      type: Boolean,
      default: false
    },
    batteryChannelLoading: {
      type: Boolean,
      default: false
    }
  },
  computed: {
    chargingStatusText() {
      if (this.chargingLoading) return '同步中'
      return this.isCharging ? '正在充电' : '充电关闭'
    },
    batteryChannelStatusText() {
      if (this.batteryChannelLoading) return '同步中'
      return this.batteryChannelEnabled ? '电池 2' : '电池 1'
    },
    batteryChannelMetaText() {
      return `当前通道：${this.batteryChannelEnabled ? '电池 2' : '电池 1'}`
    }
  },
  methods: {
    normalizeSwitchValue(value) {
      if (typeof value === 'boolean') return value
      if (typeof value === 'number') return value === 1
      if (typeof value === 'string') {
        const normalized = value.trim().toLowerCase()
        if (normalized === 'true' || normalized === '1') return true
        if (normalized === 'false' || normalized === '0') return false
      }
      return Boolean(value)
    },
    emitChargingToggle(event) {
      this.$emit('toggle-charging', this.normalizeSwitchValue(event.detail.value))
    },
    emitBatteryChannelToggle(event) {
      this.$emit('toggle-battery-channel', this.normalizeSwitchValue(event.detail.value))
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.control-card {
  margin-top: $space-xl;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18rpx;
}

.control-panel {
  @include glass-card(26rpx 24rpx);
  min-width: 0;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 24rpx;
}

.control-panel__header {
  min-width: 0;
  display: flex;
  align-items: flex-start;
  gap: 18rpx;
}

.control-panel__icon {
  width: 76rpx;
  height: 76rpx;
  border-radius: 22rpx;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow:
    inset 0 1rpx 0 rgba(255, 255, 255, 0.08),
    0 0 24rpx rgba(255, 186, 61, 0.14);
}

.control-panel__icon--solar {
  background: rgba(255, 194, 61, 0.12);
  border: 1rpx solid rgba(255, 213, 118, 0.2);
}

.control-panel__icon--battery {
  background: rgba(57, 255, 136, 0.1);
  border: 1rpx solid rgba(95, 255, 165, 0.2);
  box-shadow:
    inset 0 1rpx 0 rgba(255, 255, 255, 0.08),
    0 0 24rpx rgba(57, 255, 136, 0.12);
}

.control-panel__text {
  min-width: 0;
}

.control-panel__title {
  display: block;
  font-size: 28rpx;
  line-height: 1.25;
  font-weight: 600;
  color: $color-text-primary;
}

.control-panel__status {
  display: block;
  margin-top: 10rpx;
  font-size: 22rpx;
  line-height: 1.2;
  color: rgba(191, 216, 255, 0.68);
  white-space: nowrap;
}

.control-panel__status--active {
  color: $color-accent-success;
}

.control-panel__footer {
  min-width: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.control-panel__meta {
  min-width: 0;
  font-size: 20rpx;
  line-height: 1.2;
  color: rgba(218, 232, 255, 0.68);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.sun-icon {
  position: relative;
  width: 40rpx;
  height: 40rpx;
}

.sun-icon__core {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 24rpx;
  height: 24rpx;
  border-radius: 50%;
  background: linear-gradient(180deg, #ffd966 0%, #ffb020 100%);
  transform: translate(-50%, -50%);
  box-shadow: 0 0 18rpx rgba(255, 196, 64, 0.7);
}

.sun-icon__ray {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 4rpx;
  height: 12rpx;
  border-radius: 999rpx;
  background: rgba(255, 214, 92, 0.92);
  transform-origin: center -8rpx;
  box-shadow: 0 0 12rpx rgba(255, 196, 64, 0.45);
}

.sun-icon__ray--1 {
  transform: translate(-50%, -50%) rotate(0deg);
}

.sun-icon__ray--2 {
  transform: translate(-50%, -50%) rotate(45deg);
}

.sun-icon__ray--3 {
  transform: translate(-50%, -50%) rotate(90deg);
}

.sun-icon__ray--4 {
  transform: translate(-50%, -50%) rotate(135deg);
}

.sun-icon__ray--5 {
  transform: translate(-50%, -50%) rotate(180deg);
}

.sun-icon__ray--6 {
  transform: translate(-50%, -50%) rotate(225deg);
}

.sun-icon__ray--7 {
  transform: translate(-50%, -50%) rotate(270deg);
}

.sun-icon__ray--8 {
  transform: translate(-50%, -50%) rotate(315deg);
}

.channel-icon {
  position: relative;
  display: flex;
  align-items: center;
}

.channel-icon__body {
  width: 38rpx;
  height: 24rpx;
  padding: 4rpx;
  border-radius: 8rpx;
  border: 3rpx solid rgba(244, 251, 255, 0.86);
  box-sizing: border-box;
  background: rgba(244, 251, 255, 0.08);
}

.channel-icon__fill {
  width: 48%;
  height: 100%;
  border-radius: 4rpx;
  background: $color-accent-blue;
  transition: width 0.25s ease, background 0.25s ease;
}

.channel-icon__fill--active {
  width: 100%;
  background: $color-accent-success;
}

.channel-icon__cap {
  width: 5rpx;
  height: 12rpx;
  border-radius: 0 4rpx 4rpx 0;
  background: rgba(244, 251, 255, 0.86);
}

.control-switch {
  position: relative;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.control-switch ::v-deep .uni-switch-input,
.control-switch ::v-deep .wx-switch-input {
  background: rgba(49, 57, 72, 0.92) !important;
  border-color: rgba(49, 57, 72, 0.92) !important;
  box-shadow: 0 0 0 transparent !important;
  transition: background-color 0.25s ease, border-color 0.25s ease, box-shadow 0.25s ease !important;
}

.control-switch--active ::v-deep .uni-switch-input,
.control-switch--active ::v-deep .wx-switch-input {
  background: #007aff !important;
  border-color: #007aff !important;
  box-shadow: 0 0 10px #007aff !important;
}

.control-switch--battery.control-switch--active ::v-deep .uni-switch-input,
.control-switch--battery.control-switch--active ::v-deep .wx-switch-input {
  background: #39ff88 !important;
  border-color: #39ff88 !important;
  box-shadow: 0 0 10px rgba(57, 255, 136, 0.78) !important;
}

.control-switch ::v-deep .uni-switch-input::after,
.control-switch ::v-deep .wx-switch-input::after {
  background: #f7fbff !important;
  box-shadow: 0 4rpx 14rpx rgba(4, 11, 28, 0.3);
}

.control-switch--loading {
  opacity: 0.92;
}

.control-switch--loading ::v-deep .uni-switch-input,
.control-switch--loading ::v-deep .wx-switch-input {
  transform: scale(0.98);
}

.control-switch__loading {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 22rpx;
  height: 22rpx;
  margin-top: -11rpx;
  margin-left: -11rpx;
  border-radius: 50%;
  border: 3rpx solid rgba(255, 255, 255, 0.25);
  border-top-color: rgba(255, 255, 255, 0.95);
  pointer-events: none;
  animation: switch-loading 0.65s linear infinite;
}

@keyframes switch-loading {
  from {
    transform: rotate(0deg);
  }

  to {
    transform: rotate(360deg);
  }
}

@media screen and (max-width: 480px) {
  .control-card {
    margin-top: 22rpx;
    gap: 14rpx;
  }

  .control-panel {
    padding: 22rpx 18rpx;
    gap: 18rpx;
  }

  .control-panel__header {
    gap: 14rpx;
  }

  .control-panel__icon {
    width: 64rpx;
    height: 64rpx;
    border-radius: 18rpx;
  }

  .control-panel__title {
    font-size: 24rpx;
  }

  .control-panel__status {
    margin-top: 8rpx;
    font-size: 20rpx;
  }

  .control-panel__footer {
    gap: 12rpx;
  }

  .control-panel__meta {
    font-size: 18rpx;
  }

  .sun-icon {
    transform: scale(0.82);
  }

  .channel-icon {
    transform: scale(0.9);
  }
}

@media screen and (max-width: 360px) {
  .control-card {
    grid-template-columns: 1fr;
  }

  .control-panel {
    min-height: 154rpx;
  }
}
</style>
