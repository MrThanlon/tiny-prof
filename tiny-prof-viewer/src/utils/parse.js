// SPDX-License-Identifier: AGPL-3.0-or-later
function randomColor() {
    let ret = '#'
    for (let i = 0; i < 3; i++)
        ret += Math.round(Math.random() * 16).toString(16)
    return '#ccc'
}

/**
 * @param {DataView} profile
 * @param {Map<BigInt, String>} [symbolMap=new Map()] 
 */
export function parse(profile, symbolMap = new Map()) {
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
    const ENTER_MASK = 1n << 63n
    const TIMEU_MASK = (1n << 63n) - 1n
    // read sizes
    const sizes = profile.getUint32(4, isLittleEndian)
    const pointerSize = sizes & 0b1111
    const pthreadSize = (sizes >> 4) & 0b1111
    console.debug(`pointerSize: ${pointerSize}, pthreadSize: ${pthreadSize}`)
    // read traceBegin address
    const traceBeginAddress = profile.getBigUint64(8, isLittleEndian)
    let addr
    symbolMap.forEach((v, k) => {
        if (v === 'traceBegin') {
            addr = k
        }
    })
    const funcOffset = addr ? (traceBeginAddress - addr) : 0n
    console.debug(`traceBegin:${traceBeginAddress.toString(16)}, offset: ${funcOffset.toString(16)}`)

    let startUsec = Infinity
    let endUsec = 0n
    const threads = new Map()

    for (let offset = 16; offset < profile.byteLength;) {
        // read records info
        // read pthread_t
        let tid
        if (pthreadSize == 8) {
            tid = profile.getBigUint64(offset, isLittleEndian)
            offset += 8
        } else if (pthreadSize == 4) {
            tid = BitInt(profile.getUint32(offset, isLittleEndian))
            offset += 4
        }
        let thread
        if (threads.has(tid)) {
            thread = threads.get(tid)
        } else {
            thread = {
                frame: [],
                top: null
            }
            threads.set(tid, thread)
        }
        // read records size
        const blockSize = profile.getBigUint64(offset, isLittleEndian)
        offset += 8
        // read records
        for (let i = 0n; i < blockSize; i++) {
            // read time_usec
            const tu = profile.getBigUint64(offset, isLittleEndian)
            offset += 8
            // read func
            let func
            if (pointerSize === 8) {
                func = profile.getBigUint64(offset, isLittleEndian) - funcOffset
            } else if (pointerSize === 4) {
                func = profile.getUint32(offset, isLittleEndian) - funcOffset
            }
            offset += pointerSize
            const timeUsec = tu & TIMEU_MASK
            if (endUsec < timeUsec) {
                endUsec = timeUsec
            }
            if (tu & ENTER_MASK) {
                // enter func
                if (startUsec > timeUsec) {
                    startUsec = timeUsec
                }
                const frame = {
                    func,
                    name: symbolMap.get(func) || `<${func.toString(16)}>`,
                    start: timeUsec,
                    duration: null,
                    //color: randomColor(),
                    caller: thread.top,
                    children: []
                }
                if (thread.top) {
                    thread.top.children.push(frame)
                } else {
                    thread.frame.push(frame)
                }
                thread.top = frame
            } else {
                if (!thread.top) {
                    // ??? broken stacks
                    console.warn(`expected enter func , got exit ${func}, stacks might be broken`)
                    continue
                }
                // exit func
                const frame = thread.top
                //console.debug(`exit thread ${tid} func ${func.toString(16)} at ${timeUsec - startUsec}`)
                if (func != frame.func) {
                    // ??? broken stacks
                    console.warn(`expected exit func ${frame.func}, got ${func}, stacks might be broken`)
                    continue
                }
                frame.duration = timeUsec - frame.start
                thread.top = frame.caller
            }
        }
    }
    const statistics = new Map()
    // post process time from bigint to number
    threads.forEach(({ frame }, key) => {
        frame.forEach(func => frameBigIntToNumber(func, startUsec, endUsec))
        frame.reduce(stat, statistics)
    })
    // statistics
    return { threads, statistics }
}

function stat(stats, func) {
    func.children.reduce(stat, stats)
    const childrenDuration = func.children.reduce((pre, cur) => pre + cur.duration, 0)
    if (stats.has(func.func)) {
        const info = stats.get(func.func)
        info.call += 1
        info.total += func.duration
        info.self += func.duration - childrenDuration
    } else {
        stats.set(func.func, {
            name: func.name,
            total: func.duration,
            self: func.duration - childrenDuration,
            call: 1
        })
    }
    return stats
}

function frameBigIntToNumber (func, startUsec, endUsec) {
    if (func.duration === null) {
        func.duration = endUsec - func.start
    }
    func.start = Number(func.start - startUsec) / 1000
    func.duration = Number(func.duration) / 1000
    func.children.forEach((func => frameBigIntToNumber(func, startUsec, endUsec)))
}
