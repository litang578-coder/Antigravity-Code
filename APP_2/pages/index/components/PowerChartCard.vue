<template>
  <view class="chart-card">
    <view class="chart-card__header">
      <view class="chart-card__title-block">
        <text class="chart-card__title">最近 30 秒功率波动图</text>
        <text class="chart-card__subtitle">基于最新 30 个采样点绘制</text>
        <view v-if="isDataReady" class="chart-card__legend">
          <view class="chart-card__legend-item">
            <view class="chart-card__legend-swatch chart-card__legend-swatch--power"></view>
            <text>实际功率</text>
          </view>
          <view class="chart-card__legend-item">
            <view class="chart-card__legend-swatch chart-card__legend-swatch--mppt"></view>
            <text>MPPT输出</text>
          </view>
        </view>
      </view>
      <view class="chart-card__latest-list" :class="{ 'chart-card__latest-list--placeholder': !isDataReady }">
        <text class="chart-card__latest">
          {{ isDataReady ? `实际 ${formattedPower} W` : '连接中...' }}
        </text>
        <text v-if="isDataReady" class="chart-card__latest chart-card__latest--mppt">
          MPPT {{ formattedCompensatedPower }} W
        </text>
      </view>
    </view>

    <view class="chart-card__plot">
      <canvas
        id="power-chart"
        canvas-id="power-chart"
        class="chart-card__canvas"
      ></canvas>
    </view>
  </view>
</template>

<script>
const CHART_DRAW_THROTTLE_DELAY = 300

export default {
  name: 'PowerChartCard',
  props: {
    power: {
      type: [Number, String],
      default: 0
    },
    compensatedPower: {
      type: [Number, String],
      default: null
    },
    isDataReady: {
      type: Boolean,
      default: false
    },
    powerHistory: {
      type: Array,
      default: () => []
    },
    compensatedPowerHistory: {
      type: Array,
      default: () => []
    }
  },
  data() {
    return {
      chartContext: null,
      chartWidth: 0,
      chartHeight: 0,
      drawTimer: null,
      lastDrawAt: 0,
      initTimer: null
    }
  },
  computed: {
    formattedPower() {
      return this.formatPowerValue(this.power)
    },
    formattedCompensatedPower() {
      return this.formatPowerValue(this.compensatedPower)
    }
  },
  watch: {
    powerHistory() {
      this.$nextTick(() => this.scheduleDraw())
    },
    compensatedPowerHistory() {
      this.$nextTick(() => this.scheduleDraw())
    }
  },
  mounted() {
    this.$nextTick(() => {
      this.initChart()
      this.initTimer = setTimeout(() => this.initChart(), 80)
    })
  },
  beforeDestroy() {
    if (this.initTimer) {
      clearTimeout(this.initTimer)
      this.initTimer = null
    }
    if (this.drawTimer) {
      clearTimeout(this.drawTimer)
      this.drawTimer = null
    }
    this.destroyChart()
  },
  methods: {
    scheduleDraw() {
      if (!this.chartContext || !this.chartWidth || !this.chartHeight) return

      const now = Date.now()
      const elapsed = now - this.lastDrawAt

      if (elapsed >= CHART_DRAW_THROTTLE_DELAY) {
        if (this.drawTimer) {
          clearTimeout(this.drawTimer)
          this.drawTimer = null
        }
        this.drawChart()
        return
      }

      if (this.drawTimer) return

      this.drawTimer = setTimeout(() => {
        this.drawTimer = null
        this.drawChart()
      }, Math.max(CHART_DRAW_THROTTLE_DELAY - elapsed, 0))
    },
    initChart() {
      const query = uni.createSelectorQuery().in(this)
      query
        .select('#power-chart')
        .fields({ size: true })
        .exec((res) => {
          const canvasInfo = res && res[0]
          if (!canvasInfo || !canvasInfo.width || !canvasInfo.height) return

          const ctx = uni.createCanvasContext('power-chart', this)
          this.chartContext = ctx
          this.chartWidth = canvasInfo.width
          this.chartHeight = canvasInfo.height

          this.drawChart()
        })
    },
    destroyChart() {
      if (this.chartContext && this.chartWidth && this.chartHeight) {
        this.chartContext.clearRect(0, 0, this.chartWidth, this.chartHeight)
      }

      this.flushChart()

      this.chartContext = null
      this.chartWidth = 0
      this.chartHeight = 0
      this.lastDrawAt = 0
    },
    isBlankValue(value) {
      return value === null || value === undefined || value === ''
    },
    formatPowerValue(value) {
      if (this.isBlankValue(value)) return '--'
      const num = Number(value)
      if (Number.isNaN(num)) return '--'
      return num.toFixed(3)
    },
    getNumericPoints(points) {
      return points
        .slice(-30)
        .map((value) => Number(value))
        .filter((value) => !Number.isNaN(value))
    },
    setFillStyle(ctx, value) {
      if (ctx.setFillStyle) {
        ctx.setFillStyle(value)
      } else {
        ctx.fillStyle = value
      }
    },
    setStrokeStyle(ctx, value) {
      if (ctx.setStrokeStyle) {
        ctx.setStrokeStyle(value)
      } else {
        ctx.strokeStyle = value
      }
    },
    setLineWidth(ctx, value) {
      if (ctx.setLineWidth) {
        ctx.setLineWidth(value)
      } else {
        ctx.lineWidth = value
      }
    },
    setLineJoin(ctx, value) {
      if (ctx.setLineJoin) {
        ctx.setLineJoin(value)
      } else {
        ctx.lineJoin = value
      }
    },
    setLineCap(ctx, value) {
      if (ctx.setLineCap) {
        ctx.setLineCap(value)
      } else {
        ctx.lineCap = value
      }
    },
    setShadow(ctx, color, blur) {
      if (ctx.setShadow) {
        ctx.setShadow(0, 0, blur, color)
      } else {
        ctx.shadowColor = color
        ctx.shadowBlur = blur
      }
    },
    clearShadow(ctx) {
      if (ctx.setShadow) {
        ctx.setShadow(0, 0, 0, 'rgba(0, 0, 0, 0)')
      } else {
        ctx.shadowBlur = 0
      }
    },
    flushChart() {
      if (this.chartContext && this.chartContext.draw) {
        this.chartContext.draw()
      }
    },
    buildSmoothPath(ctx, coords) {
      ctx.beginPath()
      ctx.moveTo(coords[0].x, coords[0].y)

      for (let i = 0; i < coords.length - 1; i += 1) {
        const current = coords[i]
        const next = coords[i + 1]
        const midX = (current.x + next.x) / 2
        const midY = (current.y + next.y) / 2
        ctx.quadraticCurveTo(current.x, current.y, midX, midY)
      }

      const lastPoint = coords[coords.length - 1]
      ctx.lineTo(lastPoint.x, lastPoint.y)
    },
    drawPoint(ctx, point, fillColor, shadowColor) {
      ctx.beginPath()
      this.setFillStyle(ctx, fillColor)
      this.setShadow(ctx, shadowColor, 18)
      ctx.arc(point.x, point.y, 4, 0, Math.PI * 2)
      ctx.fill()
      this.clearShadow(ctx)
    },
    drawSeries(ctx, coords, color, shadowColor) {
      if (!coords.length) return

      if (coords.length === 1) {
        this.drawPoint(ctx, coords[0], color, shadowColor)
        return
      }

      this.buildSmoothPath(ctx, coords)
      this.setStrokeStyle(ctx, color)
      this.setLineWidth(ctx, 3)
      this.setLineJoin(ctx, 'round')
      this.setLineCap(ctx, 'round')
      this.setShadow(ctx, shadowColor, 16)
      ctx.stroke()
      this.clearShadow(ctx)

      this.drawPoint(ctx, coords[coords.length - 1], color, shadowColor)
    },
    drawChart() {
      if (!this.chartContext || !this.chartWidth || !this.chartHeight) return
      this.lastDrawAt = Date.now()

      const ctx = this.chartContext
      const width = this.chartWidth
      const height = this.chartHeight
      const padding = {
        top: 16,
        right: 14,
        bottom: 18,
        left: 14
      }

      ctx.clearRect(0, 0, width, height)

      const baseGradient = ctx.createLinearGradient(0, 0, 0, height)
      baseGradient.addColorStop(0, 'rgba(57, 255, 136, 0.14)')
      baseGradient.addColorStop(1, 'rgba(71, 184, 255, 0.02)')
      this.setFillStyle(ctx, baseGradient)
      ctx.fillRect(0, 0, width, height)

      const chartWidth = width - padding.left - padding.right
      const chartHeight = height - padding.top - padding.bottom

      this.setStrokeStyle(ctx, 'rgba(143, 205, 255, 0.14)')
      this.setLineWidth(ctx, 1)
      for (let i = 0; i < 4; i += 1) {
        const y = padding.top + (chartHeight / 3) * i
        ctx.beginPath()
        ctx.moveTo(padding.left, y)
        ctx.lineTo(width - padding.right, y)
        ctx.stroke()
      }

      const points = this.getNumericPoints(this.powerHistory)
      const compensatedPoints = this.getNumericPoints(this.compensatedPowerHistory)
      const allPoints = points.concat(compensatedPoints)
      if (!allPoints.length) {
        this.flushChart()
        return
      }

      let minVal = Math.min(...allPoints)
      let maxVal = Math.max(...allPoints)

      if (maxVal === minVal) {
        maxVal += 0.01
        minVal -= 0.01
      }

      const range = maxVal - minVal
      const safeMin = minVal - range * 0.15
      const safeMax = maxVal + range * 0.15
      const safeRange = safeMax - safeMin || 1

      const buildCoords = (sourcePoints) => {
        const stepX = chartWidth / Math.max(sourcePoints.length - 1, 1)
        return sourcePoints.map((value, index) => {
          const x = sourcePoints.length === 1
            ? padding.left + chartWidth / 2
            : padding.left + stepX * index
          const ratio = (value - safeMin) / safeRange
          const y = padding.top + chartHeight - ratio * chartHeight
          return { x, y }
        })
      }

      const coords = buildCoords(points)
      const compensatedCoords = buildCoords(compensatedPoints)

      if (coords.length > 1) {
        this.buildSmoothPath(ctx, coords)
        ctx.lineTo(coords[coords.length - 1].x, height - padding.bottom)
        ctx.lineTo(coords[0].x, height - padding.bottom)
        ctx.closePath()

        const areaGradient = ctx.createLinearGradient(0, padding.top, 0, height - padding.bottom)
        areaGradient.addColorStop(0, 'rgba(57, 255, 136, 0.24)')
        areaGradient.addColorStop(1, 'rgba(71, 184, 255, 0.02)')
        this.setFillStyle(ctx, areaGradient)
        ctx.fill()
      }

      this.drawSeries(ctx, coords, '#4cff95', 'rgba(57, 255, 136, 0.9)')
      this.drawSeries(ctx, compensatedCoords, '#69c7ff', 'rgba(105, 199, 255, 0.85)')
      this.flushChart()
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles/dashboard.scss';

.chart-card {
  margin-top: $space-xl;
  @include glass-card(28rpx 26rpx 22rpx);
}

.chart-card__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: $space-md;
  margin-bottom: 18rpx;
}

.chart-card__title-block {
  min-width: 0;
}

.chart-card__title {
  display: block;
  font-size: 30rpx;
  font-weight: 600;
  color: $color-text-primary;
}

.chart-card__subtitle {
  display: block;
  margin-top: $space-sm;
  font-size: 22rpx;
  color: rgba(191, 216, 255, 0.65);
}

.chart-card__legend {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 14rpx;
  margin-top: 14rpx;
}

.chart-card__legend-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
  font-size: 20rpx;
  color: rgba(218, 232, 255, 0.72);
}

.chart-card__legend-swatch {
  width: 18rpx;
  height: 8rpx;
  border-radius: 999rpx;
}

.chart-card__legend-swatch--power {
  background: #4cff95;
  box-shadow: 0 0 12rpx rgba(57, 255, 136, 0.58);
}

.chart-card__legend-swatch--mppt {
  background: $color-accent-blue-strong;
  box-shadow: 0 0 12rpx rgba(105, 199, 255, 0.58);
}

.chart-card__latest-list {
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 8rpx;
}

.chart-card__latest {
  display: block;
  font-size: 24rpx;
  color: $color-accent-green;
  white-space: nowrap;
}

.chart-card__latest--mppt {
  color: $color-accent-blue-strong;
}

.chart-card__latest-list--placeholder .chart-card__latest {
  color: $color-text-placeholder;
}

.chart-card__plot {
  width: 100%;
  height: 250rpx;
  border-radius: 22rpx;
  box-sizing: border-box;
  overflow: hidden;
  background: linear-gradient(180deg, rgba(57, 255, 136, 0.14) 0%, rgba(71, 184, 255, 0.03) 100%);
}

.chart-card__canvas {
  display: block;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
}

@media screen and (min-width: 768px) {
  .chart-card__plot {
    height: 240rpx;
  }
}

@media screen and (max-width: 480px) {
  .chart-card {
    margin-top: 22rpx;
    padding: 24rpx 22rpx 20rpx;
  }

  .chart-card__header {
    gap: 12rpx;
    margin-bottom: 18rpx;
  }

  .chart-card__title {
    font-size: 26rpx;
  }

  .chart-card__subtitle {
    font-size: 20rpx;
  }

  .chart-card__legend {
    gap: 10rpx;
    margin-top: 12rpx;
  }

  .chart-card__legend-item {
    font-size: 18rpx;
  }

  .chart-card__latest {
    font-size: 20rpx;
  }

  .chart-card__plot {
    height: 220rpx;
    border-radius: 20rpx;
  }
}

@media screen and (max-width: 360px) {
  .chart-card__header {
    flex-direction: column;
  }

  .chart-card__latest-list {
    align-items: flex-start;
  }

  .chart-card__plot {
    height: 200rpx;
  }
}
</style>
