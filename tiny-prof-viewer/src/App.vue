<script setup>
import {
  FlameChart,
  FlameChartContainer, TimeGridPlugin,
  FlameChartPlugin, MarksPlugin,
  UIPlugin, TogglePlugin
} from 'flame-chart-js'
import { parse } from './utils/parse'
import Module from './elfsym/elfsym.js'
import { ref, onMounted } from 'vue'

function fileToDataView (file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = e => resolve(new DataView(e.target.result))
    reader.onerror = e => reject(e)
    reader.readAsArrayBuffer(file)
  })
}

function selectFile (func) {
  const input = document.createElement('input')
  input.type = 'file'
  input.addEventListener('change', async e => {
    func(await fileToDataView(input.files[0]))
  })
  input.click()
}

let flameChart = null
const flame = ref()
const symbolMap = {
  traceBeginAddress: 0n,
  map: new Map()
}

/**
 * 
 * @param {DataView} data 
 */
function openProfile(data) {
  const flameData = parse(data, symbolMap)
  console.debug(flameData)
  const flameDataArray = Array.from(flameData)
  const container = flame.value.parentElement
  flame.value.width = container.offsetWidth
  flame.value.height = container.offsetHeight
  if (flameChart) {
    //flameChart.setData(flameData)
  } else {
    flameChart = new FlameChartContainer({
      canvas: flame.value,
      plugins: [
        new TimeGridPlugin(),
        ...flameDataArray.flatMap(thread => {
          return [
            new TogglePlugin(`Thread ${thread[0]}`),
            new FlameChartPlugin({ data: thread[1].frame, name: `Thread ${thread[0]}`})
          ]
        })
      ],
      settings: {
        styles: {
          main: {
            //backgroundColor: '#131313'
          }
        }
      }
    })
  }
}

const functions = ref([])

/**
 * 
 * @param {DataView} data 
 */
async function addSymbols(data) {
  const instance = await Module()
  console.debug(instance)
  const buffer = instance._malloc(data.byteLength)
  instance.HEAPU8.set(new Uint8Array(data.buffer), buffer)
  const outputBuffer = instance._elfsym_load(buffer, data.byteLength)
  instance._free(buffer)
  // parse symbols
  const outputDataView = new DataView(instance.HEAPU8.buffer)
  console.debug(outputDataView)
  let outputPtr = outputBuffer
  const totalSymbols = outputDataView.getBigUint64(outputPtr, true)
  outputPtr += 8
  const decoder = new TextDecoder()
  for (let i = 0; i < totalSymbols; i++) {
    const addr = outputDataView.getBigUint64(outputPtr, true)
    outputPtr += 8
    const nameLen = outputDataView.getUint8(outputPtr, true)
    outputPtr += 1
    const name = decoder.decode(outputDataView.buffer.slice(outputPtr, outputPtr + nameLen))
    outputPtr += nameLen
    if (name === 'traceBegin') {
      console.debug(`traceBegin:${addr.toString(16)}`)
      symbolMap.traceBeginAddress = addr
    }
    symbolMap.map.set(addr, name)
  }
  instance._free(outputBuffer)
  const fns = []
  symbolMap.map.forEach(v => fns.push(v))
  functions.value = fns.sort((a, b) => a.localeCompare(b))
  console.debug(fns)
}

</script>

<template>
  <div class="profile card">
    <canvas ref="flame"></canvas>
  </div>
  <div class="pannel">
    <div class="file card">
      <button @click="selectFile(openProfile)">Open profile</button>
      <button @click="selectFile(addSymbols)">Add symbols</button>
      <p style="text-align: center;">Symbols: {{ functions.length }}</p>
    </div>
    <div class="functions card">
    </div>
  </div>
</template>

<style scoped>
canvas {
  width: 100%;
  height: 100%;
  margin: 0;
  padding: 0;
}
.profile {
  margin: 10px;
  padding: 0;
  flex-grow: 1;
  display: flex;
  justify-content: center;
  align-items: center;
}

.pannel {
  margin: 10px;
  display: flex;
  flex-direction: column;
  max-width: 240px;
}

.file {
  margin-bottom: 10px;
  display: flex;
  flex-direction: column;
}

.functions {
  flex-grow: 1;
  overflow-y: overlay;
}
</style>
