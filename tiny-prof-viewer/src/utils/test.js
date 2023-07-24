import { parse } from './parse.js'
import { readFile } from 'fs'

readFile('../a.profile', (err, data) => {
    if (err) {
        console.error(err)
        return
    }
    const dv = new DataView(data.buffer)
    const profile = parse(dv)
    profile.threads.forEach(({frame}, tid) => {
        console.debug(`thread: ${tid}`)
        frame.forEach(func => printFunc(func))
    })
    console.debug(profile.statistics)
})

function printFunc (func, padding=0) {
    console.debug(' '.repeat(padding) + `func: ${func.func}, start: ${func.start}, dut: ${func.duration}`)
    func.children.forEach(func => printFunc(func, padding + 4))
}