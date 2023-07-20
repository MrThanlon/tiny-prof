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

/**
 * 
 * @param {DataView} data 
 */
function openProfile(data) {
  const flameData = parse(data)
  console.debug(flameData)
  if (flameChart) {
    flameChart.setData(flameData)
  } else {
    flameChart = new FlameChart({
      canvas: flame.value,
      data: flameData
    })
  }
  console.debug(flameChart)
}

/**
 * 
 * @param {DataView} data 
 */
async function addSymbols(data) {
  const instance = await Module()
  console.debug(instance)
  const buffer = instance._malloc(data.byteLength)
  console.debug(buffer)
  instance.HEAPU8.set(new Uint8Array(data.buffer), buffer)
  console.debug(instance._elfsym_load(buffer, data.byteLength))
  instance._free(buffer)
}

const functions = ['main', 'traceBegin']

</script>

<template>
  <div class="profile card">
    <canvas ref="flame" width="800" height="600"></canvas>
  </div>
  <div class="pannel">
    <div class="file card">
      <button @click="selectFile(openProfile)">Open profile</button>
      <button @click="selectFile(addSymbols)">Add symbols</button>
      <button @click="profile.push('cool')">test</button>
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
