<script setup>
// SPDX-License-Identifier: AGPL-3.0-or-later
import Split from 'split.js'
import {
  FlameChart,
  FlameChartContainer, TimeGridPlugin,
  FlameChartPlugin, MarksPlugin,
  UIPlugin, TogglePlugin
} from 'flame-chart-js'
// import { parse } from './utils/parse'
import Worker from './utils/worker?worker'
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
const flameChartRefresh = ref(true)
const flame = ref()
const symbolMap = new Map()

const threadArray = ref([])
let plugins
const profile = ref()
const pannel = ref()
const files = ref()
const threads = ref()
const functions = ref()
const statistics = ref([])

const worker = new Worker()

/**
 * @param {DataView} profile
 * @param {Map<BigInt, String>} [symbolMap=new Map()] 
 */
function parseInWorker(profile, symbolMap = new Map()) {
  worker.postMessage({ profile, symbolMap })
  return new Promise((resolve, reject) => {
    worker.onmessage = ({ data }) => {
      if (data.ok) {
        resolve(data.data)
      } else {
        reject(data.error)
      }
    }
  })
}

const parsing = ref(false)

/**
 * 
 * @param {DataView} data 
 */
async function openProfile(data) {
  parsing.value = true
  try {
    const threadMap = await parseInWorker(data, symbolMap)
    statistics.value = Array.from(threadMap.statistics)
      .map(v => v[1])
      .sort((a, b) => b.self - a.self)
    threadArray.value = Array.from(threadMap.threads)
    if (flameChart) {
    } else {
      flame.value.width = profile.value.offsetWidth
      flame.value.height = profile.value.offsetHeight
      plugins = threadArray.value.flatMap((thread, idx) => {
        return [
          new TogglePlugin(`Thread #${idx}`),
          new FlameChartPlugin({ data: thread[1].frame, name: `Thread #${idx}`})
        ]
      })
      flameChart = new FlameChartContainer({
        canvas: flame.value,
        plugins: [
          new TimeGridPlugin(),
          ...plugins
        ],
        settings: {
          styles: {
            main: {
              //backgroundColor: '#131313',
              //fontColor: '#ffffff'
            },
            timeGridPlugin: {
              //fontColor: '#ffffff'
            }
          }
        }
      })
    }
  } catch (e) {
    alert('Parse profile error: ' + e)
  }
  parsing.value = false
}

const symbols = ref([])

/**
 * 
 * @param {DataView} data 
 */
async function addSymbols(data) {
  const instance = await Module()
  const buffer = instance._malloc(data.byteLength)
  instance.HEAPU8.set(new Uint8Array(data.buffer), buffer)
  const outputBuffer = instance._elfsym_load(buffer, data.byteLength)
  instance._free(buffer)
  // parse symbols
  const outputDataView = new DataView(instance.HEAPU8.buffer)
  //console.debug(outputDataView)
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
    symbolMap.set(addr, name)
  }
  instance._free(outputBuffer)
  const fns = []
  symbolMap.forEach(v => fns.push(v))
  symbols.value = fns.sort((a, b) => a.localeCompare(b))
  //console.debug(fns)
}

onMounted(() => {
  Split([profile.value, pannel.value], {
    sizes: [70, 30],
    snapOffset: 0
  })
  Split([files.value, threads.value, functions.value], {
    direction: 'vertical',
    minSize: 0,
    snapOffset: 0
  })
})

const functionsOrder = ref({order: 'Name', direction: 'Down'})

</script>

<template>
  <div ref="profile">
    <canvas v-if="flameChartRefresh" ref="flame" width="800" height="600"></canvas>
  </div>
  <div class="pannel" ref="pannel">
    <div class="files" ref="files">
      <button :disabled="parsing" @click="selectFile(addSymbols)">Add symbols</button>
      <button :disabled="parsing" @click="selectFile(openProfile)">Open profile</button>
      <p style="text-align: center;">Symbols: {{ symbols.length }}</p>
    </div>
    <div class="threads" ref="threads">
      <p v-for="(thread, idx) in threadArray">
        <label>
          <input type="checkbox">
          Thread #{{ idx }}
        </label>
      </p>
    </div>
    <div class="functions" ref="functions">
      <table>
        <thead>
          <tr>
            <td class="table-index">Name</td>
            <td class="table-index">Calls</td>
            <td class="table-index">Total</td>
            <td class="table-index">Self&UpArrow;</td>
            <td class="table-index">Total/call</td>
            <td class="table-index">Self/call</td>
          </tr>
        </thead>
        <tbody>
          <tr v-for="func in statistics">
            <td>{{ func.name }}</td>
            <td>{{ func.call }}</td>
            <td>{{ func.total.toFixed(3) }}</td>
            <td>{{ func.self.toFixed(3) }}</td>
            <td>{{ (func.total / func.call).toFixed(3) }}</td>
            <td>{{ (func.self / func.call).toFixed(3) }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<style scoped>
canvas {
  max-width: 100%;
  max-height: 100%;
  margin: 0;
  padding: 0;
}

table {
  width: 100%;
}

.table-index {
  cursor: pointer;
}

.pannel {
  display: flex;
  flex-direction: column;
}

.files {
  margin-bottom: 10px;
  display: flex;
  flex-direction: column;
}

.threads {
  flex-grow: 1;
  overflow-y: scroll;
  margin-bottom: 10px;
}

.functions {
  flex-grow: 1;
  overflow-y: scroll;
}
</style>
