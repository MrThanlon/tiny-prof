function randomColor() {
    let ret = '#'
    for (let i = 0; i < 3; i++)
        ret += Math.round(Math.random() * 16).toString(16)
    return '#ccc'
}

/**
 * @param {DataView} profile
 * @param {{ traceBeginAddress: BigInt; map: Map<BigInt, String>; }} [symbolMap={ traceBeginAddress: 0n, map: new Map()}] 
 */
export function parse(profile, symbolMap = { traceBeginAddress: 0n, map: new Map()}) {
    let isLittleEndian = true
    // read magic number
    const magic = profile.getUint32(0, isLittleEndian)
    if (magic == 0x829900) {
        // big endian
        isLittleEndian = false
    } else if (magic != 0x9982) {
        // not a profile file
        throw 'not a profile'
    }
    console.debug(`isLittleEndian: ${isLittleEndian}`)
    // read pointer size
    const ENTER_MASK = 1n << 63n
    const TIMEU_MASK = (1n << 63n) - 1n
    const pointerSize = profile.getUint32(4, isLittleEndian)
    console.debug(`pointerSize: ${pointerSize}`)
    // read traceBegin address
    const traceBeginAddress = profile.getBigUint64(8, isLittleEndian)
    const funcOffset = symbolMap.traceBeginAddress ? traceBeginAddress - symbolMap.traceBeginAddress : 0n
    console.debug(`traceBegin:${traceBeginAddress.toString(16)}, offset: ${funcOffset.toString(16)}`)
    let startUsec = -1n
    const threads = new Map()
    let theTid = -1n

    for (let offset = 16; offset < profile.byteLength;) {
        // read time_usec
        const tu = profile.getBigUint64(offset, isLittleEndian)
        offset += 8
        // read func
        let func
        if (pointerSize == 8) {
            func = profile.getBigUint64(offset, isLittleEndian) - funcOffset
        } else if (pointerSize == 4) {
            func = profile.getUint32(offset, isLittleEndian) - funcOffset
        }
        offset += pointerSize
        // read thread id
        const tid = profile.getBigUint64(offset, isLittleEndian)
        offset += 8

        if (theTid < 0n) {
            theTid = tid
        }

        const timeUsec = tu & TIMEU_MASK
        if (tu & ENTER_MASK) {
            // enter func
            if (startUsec < 0n) {
                startUsec = timeUsec
            }
            console.debug(`ente thread ${tid} func ${func.toString(16)} at ${timeUsec - startUsec}`)

            const frame = {
                func,
                name: symbolMap.map.get(func) || `<${func.toString(16)}>`,
                start: Number(timeUsec - startUsec) / 1000,
                duration: 0,
                color: randomColor(),
                caller: null,
                children: []
            }
            if (threads.has(tid)) {
                // sub call
                const thread = threads.get(tid)
                if (thread.top) {
                    frame.caller = thread.top
                    frame.caller.children.push(frame)
                } else {
                    thread.frame.push(frame)
                }
                thread.top = frame
            } else {
                // new thread
                frame.caller = null
                const thread = {
                    frame: [frame],
                    top: frame
                }
                threads.set(tid, thread)
            }
        } else {
            // exit func
            const thread = threads.get(tid)
            const frame = thread.top
            console.debug(`exit thread ${tid} func ${func.toString(16)} at ${timeUsec - startUsec}`)
            if (func != frame.func) {
                // ??? broken stacks
                console.warn(`expected exit func ${frame.func}, got ${func}, profile might not correct`)
            }
            frame.duration = Number(timeUsec - startUsec) / 1000 - frame.start
            thread.top = frame.caller
        }
    }
    console.debug(theTid, threads)
    return threads.get(theTid).frame
}
