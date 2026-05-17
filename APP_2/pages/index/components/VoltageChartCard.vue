<template>
  <view class="chart-card">
    <view class="chart-card__header">
      <view>
        <text class="chart-card__title">最近 30 秒电压波动图</text>
        <text class="chart-card__subtitle">基于最新 30 个采样点绘制</text>
      </view>
      <text class="chart-card__latest" :class="{ 'chart-card__latest--placeholder': !isDataReady }">
        {{ isDataReady ? `当前 ${formattedVolt} V` : '连接中...' }}
      </text>
    </view>

    <view class="chart-card__plot">
      <canvas
        id="voltage-chart"
        canvas-id="voltage-chart"
        type="2d"
        class="chart-card__canvas"
      ></canvas>
    </view>
  </view>
</template>

<script>
const CHART_DRAW_THROTTLE_DELAY = 300

export default {
  name: 'VoltageChartCard',
  props: {
    volt: {
      type: [Number, String],
      default: 0
    },
    isDataReady: {
      type: Boolean,
      default: false
    },
    voltageHistory: {
      type: Array,
      default: () => []
    }
  },
  data() {
    return {
      chartNode: null,
      chartContext: null,
      chartWidth: 0,
      chartHeight: 0,
      drawTimer: null,
      lastDrawAt: 0,
      initTimer: null
    }
  },
  computed: {
    formattedVolt() {
      const num = Number(this.volt)
      if (Number.isNaN(num)) return '--'
      return num.toFixed(3)
    }
  },
  watch: {
    voltageHistory() {
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
      if (!this.chartContext || !this.chartNode || !this.chartWidth || !this.chartHeight) return

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
        .select('#voltage-chart')
        .fields({ node: true, size: true })
        .exec((res) => {
          const canvasInfo = res && res[0]
          if (!canvasInfo || !canvasInfo.node) return

          const dpr = uni.getSystemInfoSync().pixelRatio || 1
          const canvas = canvasInfo.node
          const ctx = canvas.getContext('2d')

          ctx.setTransform && ctx.setTransform(1, 0, 0, 1, 0, 0)
          this.chartNode = canvas
          this.chartContext = ctx
          this.chartWidth = canvasInfo.width
          this.chartHeight = canvasInfo.height

          canvas.width = canvasInfo.width * dpr
          canvas.height = canvasInfo.height * dpr
          ctx.scale(dpr, dpr)

          this.drawChart()
        })
    },
    destroyChart() {
      if (this.chartContext && this.chartWidth && this.chartHeight) {
        this.chartContext.clearRect(0, 0, this.chartWidth, this.chartHeight)
      }

      if (this.chartNode) {
        this.chartNode.width = 0
        this.chartNode.height = 0
      }

      this.chartNode = null
      this.chartContext = null
      this.chartWidth = 0
      this.chartHeight = 0
      this.lastDrawAt = 0
    },
    drawChart() {
      if (!this.chartContext || !this.chartNode || !this.chartWidth || !this.chartHeight) return
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
      baseGradient.addColorStop(0, 'rgba(71, 184, 255, 0.16)')
      baseGradient.addColorStop(1, 'rgba(71, 184, 255, 0.02)')
      ctx.fillStyle = baseGradient
      ctx.fillRect(0, 0, width, height)

      const chartWidth = width - padding.left - padding.right
      const chartHeight = height - padding.top - padding.bottom

      ctx.strokeStyle = 'rgba(143, 205, 255, 0.14)'
      ctx.lineWidth = 1
      for (let i = 0; i < 4; i += 1) {
        const y = padding.top + (chartHeight / 3) * i
        ctx.beginPath()
        ctx.moveTo(padding.left, y)
        ctx.lineTo(width - padding.right, y)
        ctx.stroke()
      }

      const points = this.voltageHistory.slice(-30)
      if (!points.length) return

      if (points.length === 1) {
        const x = padding.left + chartWidth / 2
        const y = padding.top + chartHeight / 2
        ctx.beginPath()
        ctx.fillStyle = '#47b8ff'
        ctx.shadowColor = 'rgba(71, 184, 255, 0.85)'
        ctx.shadowBlur = 14
        ctx.arc(x, y, 4, 0, Math.PI * 2)
        ctx.fill()
        ctx.shadowBlur = 0
        return
      }

      let minVal = Math.min(...points)
      let maxVal = Math.max(...points)

      if (maxVal === minVal) {
        maxVal += 0.01
        minVal -= 0.01
      }

      const range = maxVal - minVal
      const safeMin = minVal - range * 0.15
      const safeMax = maxVal + range * 0.15
      const safeRange = safeMax - safeMin || 1
      const stepX = chartWidth / Math.max(points.length - 1, 1)

      const coords = points.map((value, index) => {
        const x = padding.left + stepX * index
        const ratio = (value - safeMin) / safeRange
        const y = padding.top + chartHeight - ratio * chartHeight
        return { x, y }
      })

      const buildSmoothPath = () => {
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
      }

      buildSmoothPath()
      ctx.lineTo(coords[coords.length - 1].x, height - padding.bottom)
      ctx.lineTo(coords[0].x, height - padding.bottom)
      ctx.closePath()

      const areaGradient = ctx.createLinearGradient(0, padding.top, 0, height - padding.bottom)
      areaGradient.addColorStop(0, 'rgba(71, 184, 255, 0.32)')
      areaGradient.addColorStop(1, 'rgba(71, 184, 255, 0.02)')
      ctx.fillStyle = areaGradient
      ctx.fill()

      buildSmoothPath()
      ctx.strokeStyle = '#69c7ff'
      ctx.lineWidth = 3
      ctx.lineJoin = 'round'
      ctx.lineCap = 'round'
      ctx.shadowColor = 'rgba(71, 184, 255, 0.95)'
      ctx.shadowBlur = 16
      ctx.stroke()
      ctx.shadowBlur = 0

      const lastPoint = coords[coords.length - 1]
      ctx.beginPath()
      ctx.fillStyle = '#d6f5ff'
      ctx.shadowColor = 'rgba(71, 184, 255, 1)'
      ctx.shadowBlur = 18
      ctx.arc(lastPoint.x, lastPoint.y, 4, 0, Math.PI * 2)
      ctx.fill()
      ctx.shadowBlur = 0
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

.chart-card__latest {
  font-size: 24rpx;
  color: $color-accent-blue-strong;
  white-space: nowrap;
}

.chart-card__latest--placeholder {
  color: $color-text-placeholder;
}

.chart-card__plot {
  width: 100%;
  height: 250rpx;
  border-radius: 22rpx;
  box-sizing: border-box;
  overflow: hidden;
  background: linear-gradient(180deg, rgba(71, 184, 255, 0.16) 0%, rgba(71, 184, 255, 0.03) 100%);
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

  .chart-card__latest {
    font-size: 21rpx;
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

  .chart-card__plot {
    height: 200rpx;
  }
}
</style>
