<script setup>
import { FlameChart } from 'flame-chart-js'
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
  if (flameChart) {
    flameChart.setData(flameData)
  } else {
    flameChart = new FlameChart({
      canvas: flame.value,
      data: flameData
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
  const decoder = new TextDecoder()
  let count = 0
  while (true) {
    const addr = outputDataView.getBigUint64(outputPtr, true)
    if (addr === 0n || count > 1024) {
      break
    }
    outputPtr += 8
    const nameLen = outputDataView.getUint8(outputPtr, true)
    outputPtr += 1
    const name = decoder.decode(outputDataView.buffer.slice(outputPtr, outputPtr + nameLen))
    outputPtr += nameLen
    console.debug(addr.toString(16), name)
    count += 1
    if (name === 'traceBegin') {
      console.debug(`traceBegin:${addr.toString(16)}`)
      symbolMap.traceBeginAddress = addr
    }
    symbolMap.map.set(addr, name)
  }
  instance._free(outputBuffer)
  const fns = []
  symbolMap.map.forEach(v => fns.push(v))
  functions.value = fns
  console.debug(fns)
}

</script>

<template>
  <div class="profile card">
    <canvas ref="flame" width="800" height="600"></canvas>
  </div>
  <div class="pannel">
    <div class="file card">
      <button @click="selectFile(openProfile)">Open profile</button>
      <button @click="selectFile(addSymbols)">Add symbols</button>
    </div>
    <div class="functions card">
      <div v-for="func in functions">
        {{ func }}
      </div>
    </div>
  </div>
</template>

<style scoped>
.profile {
  margin: 10px;
  flex-grow: 1;
  display: flex;
  justify-content: center;
  align-items: center;
}

.pannel {
  margin: 10px;
  display: flex;
  flex-direction: column;
  min-width: 240px;
}

.file {
  margin-bottom: 10px;
  display: flex;
  flex-direction: column;
}

.functions {
  flex-grow: 1;
}
</style>
