<template>
  <view class="hero">
    <text class="hero__title">设备状态总览</text>
    <text class="hero__subtitle">实时显示 SOC、电压、电流与温度数据</text>
  </view>
</template>

<script>
export default {
  name: 'DashboardHero'
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.hero {
  margin-top: 34rpx;
  padding: $space-md 4rpx $space-sm;
}

.hero__title {
  display: block;
  font-size: 52rpx;
  line-height: 1.25;
  font-weight: 700;
  color: $color-text-primary;
  letter-spacing: 2rpx;
}

.hero__subtitle {
  display: block;
  margin-top: 14rpx;
  font-size: 26rpx;
  line-height: 1.7;
  color: $color-text-soft;
}

@media screen and (max-width: 480px) {
  .hero {
    margin-top: 26rpx;
    padding-top: 10rpx;
  }

  .hero__title {
    font-size: 42rpx;
    letter-spacing: 0;
  }

  .hero__subtitle {
    margin-top: 10rpx;
    font-size: 24rpx;
    line-height: 1.55;
  }
}
</style>
