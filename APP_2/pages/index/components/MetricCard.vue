<template>
  <view class="data-card" :class="{ 'data-card--placeholder': !ready }">
    <text class="data-card__label">{{ label }}</text>
    <view class="data-card__value-group">
      <text v-if="ready" class="data-card__value" :class="valueClass">{{ formattedValue }}</text>
      <text v-else class="data-card__value data-card__value--placeholder">连接中...</text>
      <text v-if="ready" class="data-card__unit">{{ unit }}</text>
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
    }
  },
  computed: {
    formattedValue() {
      const num = Number(this.value)
      if (Number.isNaN(num)) return '--'
      return num.toFixed(this.precision)
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
    min-height: 184rpx;
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

  .data-card__meta {
    margin-top: 14rpx;
    font-size: 19rpx;
    letter-spacing: 1rpx;
  }
}
</style>
