<script setup>
import Thread from './components/Thread.vue'
import Function from './components/Function.vue'
import {parse} from './utils/parse'
import Module from './elfsym/elfsym.js'

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

/**
 * 
 * @param {DataView} data 
 */
function openProfile(data) {
  console.debug(parse(data))
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

</script>

<template>
  <div class="stacks">
    <Thread></Thread>
  </div>
  <div class="pannel">
    <div class="file card">
      <button @click="selectFile(openProfile)">Open profile</button>
      <button @click="selectFile(addSymbols)">Add symbols</button>
    </div>
    <div class="functions card">
      <Function></Function>
    </div>
  </div>
</template>

<style scoped>
.stacks {
  margin: 10px;
  flex-grow: 1;
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
