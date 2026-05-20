<template>
  <view class="control-card">
    <view class="control-card__info">
      <view class="control-card__icon">
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
      <view class="control-card__text">
        <text class="control-card__title">太阳能充电控制</text>
        <text class="control-card__subtitle">Solar Charging Control</text>
      </view>
    </view>

    <view class="control-card__switches">
      <view class="control-switch-control">
        <text class="control-switch-control__label">太阳能</text>
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

      <view class="control-switch-control">
        <text class="control-switch-control__label">电池通道</text>
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
  methods: {
    emitChargingToggle(event) {
      this.$emit('toggle-charging', Boolean(event.detail.value))
    },
    emitBatteryChannelToggle(event) {
      this.$emit('toggle-battery-channel', Boolean(event.detail.value))
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.control-card {
  margin-top: $space-xl;
  @include glass-card;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: $space-lg;
}

.control-card__info {
  display: flex;
  align-items: center;
  gap: 24rpx;
  min-width: 0;
}

.control-card__icon {
  width: 88rpx;
  height: 88rpx;
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(255, 194, 61, 0.12);
  border: 1rpx solid rgba(255, 213, 118, 0.2);
  box-shadow:
    inset 0 1rpx 0 rgba(255, 255, 255, 0.08),
    0 0 24rpx rgba(255, 186, 61, 0.14);
}

.control-card__text {
  min-width: 0;
}

.control-card__title {
  display: block;
  font-size: 30rpx;
  font-weight: 600;
  color: $color-text-primary;
}

.control-card__subtitle {
  display: block;
  margin-top: 10rpx;
  font-size: 22rpx;
  color: rgba(191, 216, 255, 0.65);
  letter-spacing: 1rpx;
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

.control-card__switches {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  gap: 22rpx;
}

.control-switch-control {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
}

.control-switch-control__label {
  font-size: 20rpx;
  line-height: 1;
  color: rgba(218, 232, 255, 0.72);
  white-space: nowrap;
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
    padding: 24rpx 22rpx;
    gap: 18rpx;
  }

  .control-card__info {
    gap: 18rpx;
  }

  .control-card__icon {
    width: 76rpx;
    height: 76rpx;
    border-radius: 20rpx;
  }

  .control-card__title {
    font-size: 26rpx;
  }

  .control-card__subtitle {
    margin-top: 8rpx;
    font-size: 20rpx;
    letter-spacing: 0;
  }

  .sun-icon {
    transform: scale(0.88);
  }

  .control-card__switches {
    gap: 14rpx;
  }

  .control-switch-control {
    gap: 10rpx;
  }

  .control-switch-control__label {
    font-size: 18rpx;
  }
}

@media screen and (max-width: 360px) {
  .control-card {
    align-items: flex-start;
  }

  .control-card__switches {
    align-self: flex-end;
  }
}
</style>
