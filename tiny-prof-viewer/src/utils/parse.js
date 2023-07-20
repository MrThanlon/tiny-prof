function randomColor() {
    let ret = '#'
    for (let i = 0; i < 3; i++)
        ret += Math.round(Math.random() * 16).toString(16)
    return ret
}

/**
 * @param {DataView} profile
 */
export function parse(profile) {
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
    const ret = []
    let currentFrame = null
    let startUsec = -1n
    for (let offset = 16; offset < profile.byteLength;) {
        // read time_usec
        const tu = profile.getBigUint64(offset, isLittleEndian)
        offset += 8
        // read func
        let func
        if (pointerSize == 8) {
            func = profile.getBigUint64(offset, isLittleEndian)
        } else if (pointerSize == 4) {
            func = profile.getUint32(offset, isLittleEndian)
        }
        offset += pointerSize
        const timeUsec = tu & TIMEU_MASK
        if (tu & ENTER_MASK) {
            // enter func
            if (startUsec < 0n) {
                startUsec = timeUsec
            }
            console.debug('enter', func, timeUsec - startUsec)
            const next_frame = {
                func,
                name: `<${func.toString(16)}>`,
                start: Number(timeUsec - startUsec) / 1000,
                duration: 0,
                color: randomColor(),
                caller: currentFrame,
                children: []
            }
            if (currentFrame) {
                currentFrame.children.push(next_frame)
            } else {
                // root
                ret.push(next_frame)
            }
            currentFrame = next_frame
        } else {
            // exit func
            console.debug('exit', func, timeUsec - startUsec)
            if (func != currentFrame.func) {
                // ???
                console.warn(`expected exit func ${stacks[stacks.length - 1].func}, got ${func}, profile might not correct`)
            }
            currentFrame.duration = Number(timeUsec - startUsec) / 1000 - currentFrame.start
            currentFrame = currentFrame.caller
        }
    }
    return ret
}
