<template>
  <view class="page">
    <view class="page__glow page__glow--left"></view>
    <view class="page__glow page__glow--right"></view>

    <view class="dashboard">
      <dashboard-status />
      <dashboard-hero />

      <view class="card-grid">
        <battery-soc-card
          class="card-grid__main"
          :ready="isDataReady"
          :soc="battery_soc"
          :relay-bat="Relay_BAT"
          :is-charging="isCharging"
          :is-charging-up="isBatteryChargingUp"
        />

        <metric-card
          class="card-grid__metric"
          label="电压"
          :value="volt"
          :precision="3"
          unit="V"
          meta="Power Voltage"
          :ready="isDataReady"
          value-class="data-card__value--voltage"
        />

        <metric-card
          class="card-grid__metric"
          label="电流"
          :value="current"
          :precision="3"
          unit="A"
          meta="Current Output"
          :ready="isDataReady"
          value-class="data-card__value--current"
        />

        <metric-card
          class="card-grid__metric card-grid__metric--temperature"
          label="温度"
          :value="temp"
          :precision="1"
          unit="°C"
          meta="Thermal Status"
          :ready="isDataReady"
          :value-class="temperatureColorClass"
        />
      </view>

      <charging-control-card
        :is-charging="isCharging"
        :charging-loading="isChargingLoading"
        :battery-channel-enabled="Relay_BAT"
        :battery-channel-loading="isBatteryChannelLoading"
        @toggle-charging="handleToggleCharging"
        @toggle-battery-channel="handleToggleBatteryChannel"
      />

      <power-chart-card
        :power="currentPower"
        :is-data-ready="isDataReady"
        :power-history="powerHistory"
      />
    </view>
  </view>
</template>

<script>
import DashboardStatus from './components/DashboardStatus.vue'
import DashboardHero from './components/DashboardHero.vue'
import MetricCard from './components/MetricCard.vue'
import BatterySocCard from './components/BatterySocCard.vue'
import ChargingControlCard from './components/ChargingControlCard.vue'
import PowerChartCard from './components/PowerChartCard.vue'

const { createCommonToken } = require('@/key.js')

const POLL_FAST_DELAY = 700
const POLL_RETRY_DELAYS = [800, 1500, 3000, 5000]
const REQUEST_TIMEOUT = 3000
const CHARGING_CONFIRM_REFRESH_DELAY = 250
const CHARGING_CONFIRM_TIMEOUT = 2200

export default {
  components: {
    DashboardStatus,
    DashboardHero,
    MetricCard,
    BatterySocCard,
    ChargingControlCard,
    PowerChartCard
  },
  data() {
    return {
      // 实时显示的平滑值
      temp: 25,
      current: 0,
      volt: 0,
      // 云平台获取的目标原始值
      targetTemp: 25,
      targetCurrent: 0,
      targetVolt: 0,
      battery_soc: null,
      Relay_BAT: false,
      token: '',
      // 动画相关
      animationId: null,
      timer: null,
      pollingActive: false,
      isFetchingDevData: false,
      fetchRequestTask: null,
      pollFailCount: 0,
      pendingImmediatePoll: false,
      dataTimeout: null,
      hasReceivedData: false,
      isDataConnected: false,
      hasHighTempAlert: false,
      isCharging: false, // Boolean: 太阳能充电开关状态
      isBatteryChargingUp: false,
      isChargingLoading: false,
      pendingChargingState: null,
      chargingToggleTimer: null,
      isBatteryChannelLoading: false,
      pendingBatteryChannelState: null,
      batteryChannelToggleTimer: null,
      powerHistory: []
    }
  },
  computed: {
    isDataReady() {
      return this.hasReceivedData && this.isDataConnected
    },
    currentPower() {
      return this.volt * this.current
    },
    temperatureColorClass() {
      // 使用平滑后的数值来判断颜色，过渡更自然
      return this.temp >= 45
        ? 'data-card__value--temperature-hot'
        : 'data-card__value--temperature-cool'
    }
  },
  onLoad() {
    const params = {
      author_key: 'tTilxoU39eHgA69w9zCDVni/y6AwSdaZy22ecwReKLCpiYStESqXQFabiq9nGGwk',
      version: '2022-05-01',
      user_id: '434109'
    }
    this.token = createCommonToken(params)
  },
  onShow() {
    this.startSmoothing()
    this.startPolling()
  },
  onHide() {
    this.stopPageTasks()
  },
  onUnload() {
    this.stopPageTasks()
  },
  methods: {
    startPolling() {
      this.pollingActive = true
      this.pollFailCount = 0
      this.scheduleNextPoll(0)
    },
    stopPolling() {
      this.pollingActive = false

      if (this.timer) {
        clearTimeout(this.timer)
        this.timer = null
      }

      if (this.fetchRequestTask && typeof this.fetchRequestTask.abort === 'function') {
        this.fetchRequestTask.abort()
      }

      this.fetchRequestTask = null
      this.isFetchingDevData = false
      this.pendingImmediatePoll = false
    },
    scheduleNextPoll(delay = POLL_FAST_DELAY) {
      if (!this.pollingActive) return

      if (this.timer) {
        clearTimeout(this.timer)
      }

      this.timer = setTimeout(() => {
        this.timer = null
        this.fetchDevData()
      }, delay)
    },
    getNextPollDelay() {
      if (this.pollFailCount <= 0) return POLL_FAST_DELAY

      const retryIndex = Math.min(this.pollFailCount - 1, POLL_RETRY_DELAYS.length - 1)
      return POLL_RETRY_DELAYS[retryIndex]
    },
    finishFetchDevData(success) {
      this.isFetchingDevData = false
      this.fetchRequestTask = null
      this.pollFailCount = success ? 0 : this.pollFailCount + 1

      if (this.pendingImmediatePoll) {
        this.pendingImmediatePoll = false
        this.scheduleNextPoll(0)
        return
      }

      this.scheduleNextPoll(this.getNextPollDelay())
    },
    requestImmediatePoll(delay = 0) {
      if (!this.pollingActive) return

      if (this.isFetchingDevData) {
        this.pendingImmediatePoll = true
        return
      }

      this.scheduleNextPoll(delay)
    },
    // 低通滤波平滑算法逻辑
    startSmoothing() {
      if (this.animationId) return

      const step = () => {
        // 滤波系数 alpha，越小越平滑，响应越慢
        const alpha = 0.08
        const currentTarget = this.isCharging ? this.targetCurrent : 0
        const currentAlpha = this.isCharging ? alpha : 0.12

        // 更新平滑值：current = current + (target - current) * alpha
        this.temp = this.temp + (this.targetTemp - this.temp) * alpha
        this.current = this.current + (currentTarget - this.current) * currentAlpha
        this.volt = this.volt + (this.targetVolt - this.volt) * alpha

        // 递归调用
        this.animationId = requestAnimationFrame(step)
      }
      this.animationId = requestAnimationFrame(step)
    },
    stopPageTasks() {
      this.stopPolling()

      if (this.dataTimeout) {
        clearTimeout(this.dataTimeout)
        this.dataTimeout = null
      }

      if (this.animationId) {
        cancelAnimationFrame(this.animationId)
        this.animationId = null
      }

      if (this.chargingToggleTimer) {
        clearTimeout(this.chargingToggleTimer)
        this.chargingToggleTimer = null
      }

      this.pendingChargingState = null

      if (this.batteryChannelToggleTimer) {
        clearTimeout(this.batteryChannelToggleTimer)
        this.batteryChannelToggleTimer = null
      }

      this.pendingBatteryChannelState = null
      this.isChargingLoading = false
      this.isBatteryChannelLoading = false
    },
    markDataReceived() {
      this.hasReceivedData = true
      this.isDataConnected = true

      if (this.dataTimeout) {
        clearTimeout(this.dataTimeout)
      }

      this.dataTimeout = setTimeout(() => {
        this.isDataConnected = false
      }, 5000)
    },
    handleTemperatureAlert(value) {
      const temperature = Number(value)

      if (Number.isNaN(temperature)) return

      if (temperature > 50 && !this.hasHighTempAlert) {
        this.hasHighTempAlert = true
        uni.vibrateShort()
        return
      }

      if (temperature <= 50) {
        this.hasHighTempAlert = false
      }
    },
    normalizeBooleanValue(value) {
      if (typeof value === 'boolean') return value
      if (typeof value === 'number') return value === 1
      if (typeof value === 'string') {
        const normalized = value.trim().toLowerCase()
        return normalized === 'true' || normalized === '1'
      }
      return false
    },
    startChargingConfirmation(nextChargingState) {
      this.pendingChargingState = nextChargingState

      if (this.chargingToggleTimer) {
        clearTimeout(this.chargingToggleTimer)
      }

      this.chargingToggleTimer = setTimeout(() => {
        this.chargingToggleTimer = null
        this.pendingChargingState = null
        this.isChargingLoading = false
        this.requestImmediatePoll(0)
      }, CHARGING_CONFIRM_TIMEOUT)
    },
    clearChargingConfirmation() {
      if (this.chargingToggleTimer) {
        clearTimeout(this.chargingToggleTimer)
        this.chargingToggleTimer = null
      }

      this.pendingChargingState = null
      this.isChargingLoading = false
    },
    handleChargingStateFromCloud(value) {
      const nextCharging = this.normalizeBooleanValue(value)

      if (this.isChargingLoading && this.pendingChargingState !== null) {
        if (nextCharging === this.pendingChargingState) {
          this.isCharging = nextCharging
          this.clearChargingConfirmation()
        }
        return
      }

      this.isCharging = nextCharging
    },
    startBatteryChannelConfirmation(nextBatteryChannelState) {
      this.pendingBatteryChannelState = nextBatteryChannelState

      if (this.batteryChannelToggleTimer) {
        clearTimeout(this.batteryChannelToggleTimer)
      }

      this.batteryChannelToggleTimer = setTimeout(() => {
        this.batteryChannelToggleTimer = null
        this.pendingBatteryChannelState = null
        this.isBatteryChannelLoading = false
        this.requestImmediatePoll(0)
      }, CHARGING_CONFIRM_TIMEOUT)
    },
    clearBatteryChannelConfirmation() {
      if (this.batteryChannelToggleTimer) {
        clearTimeout(this.batteryChannelToggleTimer)
        this.batteryChannelToggleTimer = null
      }

      this.pendingBatteryChannelState = null
      this.isBatteryChannelLoading = false
    },
    handleBatteryChannelStateFromCloud(value) {
      const nextBatteryChannelState = this.normalizeBooleanValue(value)

      if (this.isBatteryChannelLoading && this.pendingBatteryChannelState !== null) {
        if (nextBatteryChannelState === this.pendingBatteryChannelState) {
          this.Relay_BAT = nextBatteryChannelState
          this.clearBatteryChannelConfirmation()
        }
        return
      }

      this.Relay_BAT = nextBatteryChannelState
    },
    handleToggleCharging(nextChargingState) {
      const previousChargingState = this.isCharging

      this.isCharging = nextChargingState
      this.isChargingLoading = true
      uni.vibrateShort()
      console.log(`下发指令: ${JSON.stringify({ botton1: this.isCharging })}`)

      this.startChargingConfirmation(nextChargingState)

      uni.request({
        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
        method: 'POST',
        timeout: REQUEST_TIMEOUT,
        data: {
          product_id: 'dtk3h50J6V',
          device_name: 'dachuang',
          params: {
            botton1: nextChargingState
          }
        },
        header: { authorization: this.token },
        success: () => {
          this.requestImmediatePoll(CHARGING_CONFIRM_REFRESH_DELAY)
        },
        fail: () => {
          this.clearChargingConfirmation()
          this.isCharging = previousChargingState
          uni.showToast({
            title: '控制下发失败',
            icon: 'none'
          })
        }
      })
    },
    handleToggleBatteryChannel(nextBatteryChannelState) {
      const previousBatteryChannelState = this.Relay_BAT

      this.Relay_BAT = nextBatteryChannelState
      this.isBatteryChannelLoading = true
      uni.vibrateShort()
      console.log(`下发指令: ${JSON.stringify({ Relay_BAT: this.Relay_BAT })}`)

      this.startBatteryChannelConfirmation(nextBatteryChannelState)

      uni.request({
        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
        method: 'POST',
        timeout: REQUEST_TIMEOUT,
        data: {
          product_id: 'dtk3h50J6V',
          device_name: 'dachuang',
          params: {
            Relay_BAT: nextBatteryChannelState
          }
        },
        header: { authorization: this.token },
        success: () => {
          this.requestImmediatePoll(CHARGING_CONFIRM_REFRESH_DELAY)
        },
        fail: () => {
          this.clearBatteryChannelConfirmation()
          this.Relay_BAT = previousBatteryChannelState
          uni.showToast({
            title: '电池通道控制失败',
            icon: 'none'
          })
        }
      })
    },
    updatePowerHistory(volt, current) {
      const v = Number(volt)
      const c = Number(current)

      if (Number.isNaN(v) || Number.isNaN(c)) return

      const power = v * c
      this.powerHistory = this.powerHistory.concat(power).slice(-30)
    },
    buildDevicePropertyMap(dataList) {
      return dataList.reduce((propertyMap, item) => {
        if (item && item.identifier) {
          propertyMap[item.identifier] = item.value
        }
        return propertyMap
      }, Object.create(null))
    },
    fetchDevData() {
      if (!this.pollingActive || this.isFetchingDevData) return

      this.isFetchingDevData = true
      let requestSucceeded = false

      this.fetchRequestTask = uni.request({
        url: 'https://iot-api.heclouds.com/thingmodel/query-device-property',
        method: 'GET',
        timeout: REQUEST_TIMEOUT,
        data: {
          product_id: 'dtk3h50J6V',
          device_name: 'dachuang'
        },
        header: { authorization: this.token },
        success: (res) => {
          const dataList = res && res.data && res.data.data
          if (!dataList || !Array.isArray(dataList)) return

          requestSucceeded = true
          this.markDataReceived()

          const dataMap = this.buildDevicePropertyMap(dataList)
          const getVal = (id) => {
            return Object.prototype.hasOwnProperty.call(dataMap, id) ? dataMap[id] : null
          }

          // 只更新目标值，平滑逻辑会自动跟进
          const tempVal = getVal('temp')
          if (tempVal !== null) {
            this.targetTemp = Number(tempVal)
            this.handleTemperatureAlert(tempVal)
          }

          const currentVal = getVal('current')
          if (currentVal !== null) this.targetCurrent = Number(currentVal)

          const voltVal = getVal('volt')
          if (voltVal !== null) {
            const nextVolt = Number(voltVal)
            this.targetVolt = nextVolt
            const curVal = getVal('current')
            this.updatePowerHistory(nextVolt, curVal !== null ? Number(curVal) : this.targetCurrent)
          }

          const batterySocVal = getVal('battery_soc')
          if (batterySocVal !== null) {
            const nextBatterySoc = Number(batterySocVal)
            if (!Number.isNaN(nextBatterySoc)) {
              this.isBatteryChargingUp = this.battery_soc !== null && nextBatterySoc > this.battery_soc
              this.battery_soc = nextBatterySoc
            } else {
              this.isBatteryChargingUp = false
            }
          } else {
            this.isBatteryChargingUp = false
          }

          const botton1Val = getVal('botton1')
          if (botton1Val !== null) {
            this.handleChargingStateFromCloud(botton1Val)
          }

          const relayBatVal = getVal('Relay_BAT')
          if (relayBatVal !== null) {
            this.handleBatteryChannelStateFromCloud(relayBatVal)
          }
        },
        complete: () => {
          if (!this.pollingActive) return
          this.finishFetchDevData(requestSucceeded)
        }
      })
    }
  }
}
</script>

<style lang="scss" scoped>
@import './styles/dashboard.scss';

.page {
  position: relative;
  min-height: 100vh;
  overflow: hidden;
  background: linear-gradient(180deg, #0b1f3f 0%, #060b17 50%, #010409 100%);
}

.page__glow {
  position: absolute;
  border-radius: 50%;
  filter: blur(24px);
  opacity: 0.32;
}

.page__glow--left {
  top: 8%;
  left: -120rpx;
  width: 320rpx;
  height: 320rpx;
  background: rgba(0, 184, 255, 0.3);
}

.page__glow--right {
  right: -140rpx;
  bottom: 16%;
  width: 360rpx;
  height: 360rpx;
  background: rgba(0, 255, 170, 0.18);
}

.dashboard {
  position: relative;
  z-index: 1;
  min-height: 100vh;
  padding: calc(var(--status-bar-height, 0px) + 28rpx) 24rpx 36rpx;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.card-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18rpx;
  margin-top: 24rpx;
}

.card-grid__main {
  grid-column: 1 / -1;
}

@media screen and (min-width: 768px) {
  .dashboard {
    width: 92%;
    max-width: 920rpx;
    margin: 0 auto;
    padding-left: 0;
    padding-right: 0;
  }

  .card-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: $space-lg;
  }
}

@media screen and (max-width: 480px) {
  .dashboard {
    padding-left: 22rpx;
    padding-right: 22rpx;
  }

  .card-grid__metric--temperature {
    grid-column: 1 / -1;
  }
}
</style>
