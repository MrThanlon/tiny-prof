<script setup>
import Thread from './components/Thread.vue'
import Function from './components/Function.vue'

</script>

<template>
  <div class="stacks">
    <Thread></Thread>
  </div>
  <div class="pannel">
    <div class="file card">
      <button>Select a file</button>
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
}

.functions {
  flex-grow: 1;
}
</style>
