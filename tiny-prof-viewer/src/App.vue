<script setup>
import Split from 'split.js'
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
const flameChartRefresh = ref(true)
const flame = ref()
const symbolMap = {
  traceBeginAddress: 0n,
  map: new Map()
}

const threadArray = ref([])
let plugins
const profile = ref()
const pannel = ref()
const files = ref()
const threads = ref()
const functions = ref()

/**
 * 
 * @param {DataView} data 
 */
function openProfile(data) {
  const threadMap = parse(data, symbolMap)
  console.debug(threadMap)
  threadArray.value = Array.from(threadMap)
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
            //fontColor: 'white'
          },
          timeGridPlugin: {
            //fontColor: 'white'
          }
        }
      }
    })
  }
}

const symbols = ref([])

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
  symbols.value = fns.sort((a, b) => a.localeCompare(b))
  console.debug(fns)
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

</script>

<template>
  <div ref="profile">
    <canvas v-if="flameChartRefresh" ref="flame" width="800" height="600"></canvas>
  </div>
  <div class="pannel" ref="pannel">
    <div class="files" ref="files">
      <button @click="selectFile(openProfile)">Open profile</button>
      <button @click="selectFile(addSymbols)">Add symbols</button>
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
