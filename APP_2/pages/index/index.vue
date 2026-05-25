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
          secondary-label="MPPT补偿"
          :secondary-value="Um_comp"
          secondary-unit="V"
          :secondary-precision="3"
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
          secondary-label="MPPT补偿"
          :secondary-value="Im_comp"
          secondary-unit="A"
          :secondary-precision="3"
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

        <metric-card
          class="card-grid__metric card-grid__metric--efficiency"
          label="MPPT效率"
          :value="mpptEfficiency"
          :precision="1"
          unit="%"
          meta="MPPT Efficiency"
          :ready="isDataReady"
          value-class="data-card__value--efficiency"
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
        :compensated-power="mpptPower"
        :is-data-ready="isDataReady"
        :power-history="powerHistory"
        :compensated-power-history="mpptPowerHistory"
      />

      <view
        class="debug-panel"
        :class="{
          'debug-panel--success': requestDebug.status === 'success',
          'debug-panel--error': requestDebug.status === 'error'
        }"
      >
        <view class="debug-panel__header">
          <text class="debug-panel__title">请求诊断</text>
          <text class="debug-panel__time">{{ requestDebug.updatedAt || '未开始' }}</text>
        </view>
        <text class="debug-panel__state">{{ requestDebug.title }}</text>
        <view class="debug-panel__grid">
          <view class="debug-panel__item">
            <text class="debug-panel__label">HTTP</text>
            <text class="debug-panel__value">{{ requestDebug.httpStatus || '-' }}</text>
          </view>
          <view class="debug-panel__item">
            <text class="debug-panel__label">code</text>
            <text class="debug-panel__value">{{ requestDebug.code || '-' }}</text>
          </view>
        </view>
        <text class="debug-panel__message">{{ requestDebug.detail || '-' }}</text>
        <text v-if="requestDebug.errMsg" class="debug-panel__message debug-panel__message--error">
          {{ requestDebug.errMsg }}
        </text>
      </view>
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
const SMOOTHING_DELAY = 80
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
      Um_comp: null,
      Im_comp: null,
      mpptPower: null,
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
      powerHistory: [],
      mpptPowerHistory: [],
      requestDebug: {
        status: 'idle',
        title: '等待请求',
        detail: '尚未开始请求设备数据',
        httpStatus: '',
        code: '',
        errMsg: '',
        updatedAt: ''
      }
    }
  },
  computed: {
    isDataReady() {
      return this.hasReceivedData && this.isDataConnected
    },
    currentPower() {
      return this.volt * this.current
    },
    mpptEfficiency() {
      const power = this.parseNumericValue(this.mpptPower)
      if (power === null) return null
      return (power / 1.98) * 100
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
    try {
      this.token = createCommonToken(params)
      this.setRequestDebug({
        status: 'idle',
        title: 'Token 已生成',
        detail: '等待请求设备数据',
        httpStatus: '',
        code: '',
        errMsg: ''
      })
      this.startPolling()
    } catch (err) {
      this.token = ''
      this.setRequestDebug({
        status: 'error',
        title: 'Token 生成失败',
        detail: 'App 端生成 OneNET token 失败',
        httpStatus: '',
        code: '',
        errMsg: err && err.message ? err.message : String(err)
      })
    }
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
      if (!this.token) {
        this.setRequestDebug({
          status: 'error',
          title: '无法开始请求',
          detail: 'Token 为空，请先检查 token 生成',
          httpStatus: '',
          code: '',
          errMsg: ''
        })
        return
      }

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
        this.animationId = setTimeout(step, SMOOTHING_DELAY)
      }
      step()
    },
    stopPageTasks() {
      this.stopPolling()

      if (this.dataTimeout) {
        clearTimeout(this.dataTimeout)
        this.dataTimeout = null
      }

      if (this.animationId) {
        clearTimeout(this.animationId)
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
        if (normalized === 'true' || normalized === '1') return true
        if (normalized === 'false' || normalized === '0') return false
      }
      return false
    },
    getSetPropertyErrorMessage(res) {
      const statusCode = res && res.statusCode
      const data = res && res.data ? res.data : {}
      const code = data.code
      const message = data.msg || data.message || data.errmsg || ''

      if (statusCode !== 200) {
        return `HTTP ${statusCode || '异常'}`
      }

      if (code !== 0 && code !== '0') {
        return message || `云端返回码 ${code === undefined ? '缺失' : code}`
      }

      return ''
    },
    showControlFailToast(title, message) {
      uni.showToast({
        title: message ? `${title}：${message}` : title,
        icon: 'none'
      })
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
      const requestData = {
        product_id: 'dtk3h50J6V',
        device_name: 'dachuang',
        params: {
          botton1: nextChargingState
        }
      }

      this.isCharging = nextChargingState
      this.isChargingLoading = true
      uni.vibrateShort()
      console.log('[太阳能充电] 下发参数:', JSON.stringify(requestData.params))

      this.startChargingConfirmation(nextChargingState)

      uni.request({
        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
        method: 'POST',
        timeout: REQUEST_TIMEOUT,
        data: requestData,
        header: { authorization: this.token },
        success: (res) => {
          console.log('[太阳能充电] 云端响应:', {
            statusCode: res.statusCode,
            data: res.data
          })

          const errorMessage = this.getSetPropertyErrorMessage(res)
          if (errorMessage) {
            this.clearChargingConfirmation()
            this.isCharging = previousChargingState
            this.showControlFailToast('控制下发失败', errorMessage)
            return
          }

          this.requestImmediatePoll(CHARGING_CONFIRM_REFRESH_DELAY)
        },
        fail: (err) => {
          console.log('[太阳能充电] 请求失败:', err)
          this.clearChargingConfirmation()
          this.isCharging = previousChargingState
          this.showControlFailToast('控制下发失败', err && err.errMsg ? err.errMsg : '')
        }
      })
    },
    handleToggleBatteryChannel(nextBatteryChannelState) {
      const previousBatteryChannelState = this.Relay_BAT
      const requestData = {
        product_id: 'dtk3h50J6V',
        device_name: 'dachuang',
        params: {
          Relay_BAT: nextBatteryChannelState
        }
      }

      this.Relay_BAT = nextBatteryChannelState
      this.isBatteryChannelLoading = true
      uni.vibrateShort()
      console.log('[电池通道] 下发参数:', JSON.stringify(requestData.params))

      this.startBatteryChannelConfirmation(nextBatteryChannelState)

      uni.request({
        url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
        method: 'POST',
        timeout: REQUEST_TIMEOUT,
        data: requestData,
        header: { authorization: this.token },
        success: (res) => {
          console.log('[电池通道] 云端响应:', {
            statusCode: res.statusCode,
            data: res.data
          })

          const errorMessage = this.getSetPropertyErrorMessage(res)
          if (errorMessage) {
            this.clearBatteryChannelConfirmation()
            this.Relay_BAT = previousBatteryChannelState
            this.showControlFailToast('电池通道控制失败', errorMessage)
            return
          }

          this.requestImmediatePoll(CHARGING_CONFIRM_REFRESH_DELAY)
        },
        fail: (err) => {
          console.log('[电池通道] 请求失败:', err)
          this.clearBatteryChannelConfirmation()
          this.Relay_BAT = previousBatteryChannelState
          this.showControlFailToast('电池通道控制失败', err && err.errMsg ? err.errMsg : '')
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
    parseNumericValue(value) {
      if (value === null || value === undefined || value === '') return null
      const num = Number(value)
      if (Number.isNaN(num)) return null
      return num
    },
    updateMpptPowerHistory(power) {
      const nextPower = this.parseNumericValue(power)
      if (nextPower === null) return

      this.mpptPowerHistory = this.mpptPowerHistory.concat(nextPower).slice(-30)
    },
    buildDevicePropertyMap(dataList) {
      return dataList.reduce((propertyMap, item) => {
        if (item && item.identifier) {
          propertyMap[item.identifier] = item.value
        }
        return propertyMap
      }, Object.create(null))
    },
    getDebugTime() {
      const now = new Date()
      const pad = (value) => String(value).padStart(2, '0')
      return `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`
    },
    formatDebugValue(value) {
      if (value === null || value === undefined || value === '') return ''

      if (typeof value === 'object') {
        try {
          return JSON.stringify(value).slice(0, 180)
        } catch (err) {
          return String(value)
        }
      }

      return String(value).slice(0, 180)
    },
    setRequestDebug(nextDebug) {
      this.requestDebug = Object.assign({}, this.requestDebug, nextDebug, {
        updatedAt: this.getDebugTime()
      })
    },
    normalizeResponseData(rawData) {
      if (typeof rawData !== 'string') return rawData

      try {
        return JSON.parse(rawData)
      } catch (err) {
        return {
          code: 'PARSE_ERROR',
          msg: '响应不是合法 JSON',
          raw: rawData
        }
      }
    },
    fetchDevData() {
      if (!this.pollingActive || this.isFetchingDevData) return

      this.isFetchingDevData = true
      let requestSucceeded = false

      this.setRequestDebug({
        status: 'pending',
        title: '正在请求 OneNET',
        detail: 'query-device-property',
        httpStatus: '',
        code: '',
        errMsg: ''
      })

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
          const responseData = this.normalizeResponseData(res && res.data)
          const httpStatus = res && res.statusCode ? String(res.statusCode) : ''
          const code = responseData && responseData.code !== undefined
            ? String(responseData.code)
            : ''
          const message = this.formatDebugValue(
            responseData && (responseData.msg || responseData.message || responseData.errmsg || responseData.raw)
          )
          const dataList = responseData && responseData.data

          if (res && res.statusCode && res.statusCode !== 200) {
            this.setRequestDebug({
              status: 'error',
              title: 'HTTP 请求异常',
              detail: message || '云端未返回成功状态',
              httpStatus,
              code,
              errMsg: ''
            })
            return
          }

          if (code && code !== '0') {
            this.setRequestDebug({
              status: 'error',
              title: 'OneNET 返回错误',
              detail: message || '云端 code 非 0',
              httpStatus,
              code,
              errMsg: ''
            })
            return
          }

          if (!dataList || !Array.isArray(dataList)) {
            console.log('[Device data] invalid response:', {
              statusCode: res && res.statusCode,
              data: responseData
            })
            this.setRequestDebug({
              status: 'error',
              title: '数据结构异常',
              detail: this.formatDebugValue(responseData),
              httpStatus,
              code,
              errMsg: ''
            })
            return
          }

          requestSucceeded = true
          this.markDataReceived()
          this.setRequestDebug({
            status: 'success',
            title: '请求成功',
            detail: `已获取 ${dataList.length} 个属性`,
            httpStatus,
            code: code || '0',
            errMsg: ''
          })

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

          const umCompVal = getVal('Um_comp')
          this.Um_comp = this.parseNumericValue(umCompVal)

          const imCompVal = getVal('Im_comp')
          this.Im_comp = this.parseNumericValue(imCompVal)

          const mpptPowerVal = getVal('power')
          this.mpptPower = this.parseNumericValue(mpptPowerVal)
          this.updateMpptPowerHistory(mpptPowerVal)

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
        fail: (err) => {
          console.log('[Device data] request failed:', err)
          this.setRequestDebug({
            status: 'error',
            title: '请求失败',
            detail: 'uni.request fail',
            httpStatus: '',
            code: '',
            errMsg: err && err.errMsg ? err.errMsg : this.formatDebugValue(err)
          })
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

.debug-panel {
  margin-top: 22rpx;
  padding: 22rpx 24rpx;
  border-radius: 24rpx;
  border: 1rpx solid rgba(149, 170, 196, 0.24);
  background: rgba(8, 18, 36, 0.66);
  box-shadow: inset 0 1rpx 0 rgba(255, 255, 255, 0.06);
}

.debug-panel--success {
  border-color: rgba(57, 255, 136, 0.28);
  background: rgba(9, 42, 34, 0.58);
}

.debug-panel--error {
  border-color: rgba(248, 113, 113, 0.32);
  background: rgba(52, 20, 30, 0.58);
}

.debug-panel__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.debug-panel__title {
  font-size: 26rpx;
  font-weight: 600;
  color: #f4fbff;
}

.debug-panel__time {
  font-size: 20rpx;
  color: rgba(191, 216, 255, 0.62);
}

.debug-panel__state {
  display: block;
  margin-top: 14rpx;
  font-size: 24rpx;
  color: rgba(226, 238, 255, 0.86);
}

.debug-panel__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14rpx;
  margin-top: 16rpx;
}

.debug-panel__item {
  min-width: 0;
  padding: 12rpx 14rpx;
  border-radius: 16rpx;
  background: rgba(255, 255, 255, 0.06);
}

.debug-panel__label,
.debug-panel__value,
.debug-panel__message {
  display: block;
}

.debug-panel__label {
  font-size: 19rpx;
  color: rgba(191, 216, 255, 0.58);
}

.debug-panel__value {
  margin-top: 6rpx;
  font-size: 24rpx;
  color: rgba(244, 251, 255, 0.9);
  word-break: break-all;
}

.debug-panel__message {
  margin-top: 14rpx;
  font-size: 21rpx;
  line-height: 1.5;
  color: rgba(191, 216, 255, 0.72);
  word-break: break-all;
}

.debug-panel__message--error {
  color: rgba(254, 202, 202, 0.92);
}

@media screen and (min-width: 768px) {
  .dashboard {
    width: 92%;
    max-width: 1120rpx;
    margin: 0 auto;
    padding-left: 0;
    padding-right: 0;
  }

  .card-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: $space-lg;
  }
}

@media screen and (max-width: 480px) {
  .dashboard {
    padding-left: 22rpx;
    padding-right: 22rpx;
  }

}
</style>
