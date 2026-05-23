<template>
  <view class="data-card" :class="{ 'data-card--placeholder': !ready }">
    <text class="data-card__label">{{ label }}</text>
    <view class="data-card__body">
      <view class="data-card__value-group">
        <text v-if="ready" class="data-card__value" :class="valueClass">{{ formattedValue }}</text>
        <text v-else class="data-card__value data-card__value--placeholder">连接中...</text>
        <text v-if="ready" class="data-card__unit">{{ unit }}</text>
      </view>

      <view v-if="ready && hasSecondary" class="data-card__secondary">
        <text class="data-card__secondary-label">{{ secondaryLabel }}</text>
        <view class="data-card__secondary-value-group">
          <text class="data-card__secondary-value">{{ formattedSecondaryValue }}</text>
          <text v-if="secondaryUnit" class="data-card__secondary-unit">{{ secondaryUnit }}</text>
        </view>
      </view>
    </view>
    <text class="data-card__meta">{{ ready ? meta : 'Waiting For Data' }}</text>
  </view>
</template>

<script>
export default {
  name: 'MetricCard',
  props: {
    label: {
      type: String,
      required: true
    },
    value: {
      type: [Number, String],
      default: 0
    },
    precision: {
      type: Number,
      default: 1
    },
    unit: {
      type: String,
      required: true
    },
    meta: {
      type: String,
      required: true
    },
    ready: {
      type: Boolean,
      default: false
    },
    valueClass: {
      type: String,
      default: ''
    },
    secondaryLabel: {
      type: String,
      default: ''
    },
    secondaryValue: {
      type: [Number, String],
      default: null
    },
    secondaryUnit: {
      type: String,
      default: ''
    },
    secondaryPrecision: {
      type: Number,
      default: 3
    }
  },
  computed: {
    hasSecondary() {
      return Boolean(
        this.secondaryLabel ||
        this.secondaryUnit ||
        !this.isBlankValue(this.secondaryValue)
      )
    },
    formattedValue() {
      return this.formatMetricValue(this.value, this.precision)
    },
    formattedSecondaryValue() {
      return this.formatMetricValue(this.secondaryValue, this.secondaryPrecision)
    }
  },
  methods: {
    isBlankValue(value) {
      return value === null || value === undefined || value === ''
    },
    formatMetricValue(value, precision) {
      if (this.isBlankValue(value)) return '--'
      const num = Number(value)
      if (Number.isNaN(num)) return '--'
      return num.toFixed(precision)
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
  .data-card__meta,
  .data-card__unit {
    color: $color-text-placeholder;
  }
}

.data-card__label {
  display: block;
  font-size: 26rpx;
  letter-spacing: 4rpx;
  color: rgba(183, 211, 247, 0.72);
}

.data-card__value-group {
  display: flex;
  align-items: flex-end;
  gap: 12rpx;
  margin-top: $space-md;
}

.data-card__body {
  min-width: 0;
}

.data-card__value {
  font-size: 78rpx;
  line-height: 1;
  font-weight: 700;
  text-shadow: 0 0 18rpx currentColor;
  transition: color 0.5s ease;
}

.data-card__value--voltage {
  color: $color-accent-blue;
}

.data-card__value--current {
  color: $color-accent-green;
}

.data-card__value--temperature-cool {
  color: $color-accent-blue;
}

.data-card__value--temperature-hot {
  color: $color-accent-hot;
}

.data-card__value--efficiency {
  color: $color-accent-success;
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

.data-card__secondary {
  min-width: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14rpx;
  margin-top: 14rpx;
  padding-top: 14rpx;
  border-top: 1rpx solid rgba(193, 228, 255, 0.12);
}

.data-card__secondary-label {
  flex-shrink: 0;
  font-size: 20rpx;
  color: rgba(191, 216, 255, 0.62);
  white-space: nowrap;
}

.data-card__secondary-value-group {
  min-width: 0;
  display: flex;
  align-items: baseline;
  justify-content: flex-end;
  gap: 6rpx;
}

.data-card__secondary-value {
  min-width: 0;
  font-size: 34rpx;
  line-height: 1;
  font-weight: 650;
  color: rgba(244, 251, 255, 0.88);
  overflow: hidden;
  text-overflow: ellipsis;
}

.data-card__secondary-unit {
  flex-shrink: 0;
  font-size: 20rpx;
  color: rgba(226, 238, 255, 0.62);
}

.data-card__meta {
  display: block;
  margin-top: $space-md;
  font-size: 22rpx;
  color: $color-text-muted;
  text-transform: uppercase;
  letter-spacing: 2rpx;
}

@media screen and (max-width: 480px) {
  .data-card {
    min-height: 210rpx;
    padding: 24rpx 22rpx;
  }

  .data-card__label {
    font-size: 23rpx;
    letter-spacing: 2rpx;
  }

  .data-card__value-group {
    gap: 8rpx;
    margin-top: 14rpx;
  }

  .data-card__value {
    font-size: 56rpx;
  }

  .data-card__value--placeholder {
    font-size: 34rpx;
  }

  .data-card__unit {
    font-size: 24rpx;
    padding-bottom: 6rpx;
  }

  .data-card__secondary {
    gap: 8rpx;
    margin-top: 12rpx;
    padding-top: 12rpx;
  }

  .data-card__secondary-label {
    font-size: 18rpx;
  }

  .data-card__secondary-value {
    font-size: 27rpx;
  }

  .data-card__secondary-unit {
    font-size: 17rpx;
  }

  .data-card__meta {
    margin-top: 14rpx;
    font-size: 19rpx;
    letter-spacing: 1rpx;
  }
}
</style>
