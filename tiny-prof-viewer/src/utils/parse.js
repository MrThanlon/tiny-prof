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
    const TIMEU_MASK = (1n << 64n) - 1n
    const pointerSize = profile.getUint32(4, isLittleEndian)
    console.debug(`pointerSize: ${pointerSize}`)
    // read traceBegin address
    const traceBeginAddress = profile.getBigUint64(8, isLittleEndian)
    const ret = []
    let current_frame = null
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
        const time_usec = tu & TIMEU_MASK
        if (tu & ENTER_MASK) {
            // enter func
            console.debug('enter', func)
            const next_frame = {
                func, start: time_usec, end: 0,
                caller: current_frame, children: []
            }
            if (current_frame) {
                current_frame.children.push(next_frame)
            } else {
                // root
                ret.push(next_frame)
            }
            current_frame = next_frame
        } else {
            // exit func
            console.debug('exit', func)
            if (func != current_frame.func) {
                // ???
                console.warn(`expected exit func ${stacks[stacks.length - 1].func}, got ${func}, profile might not correct`)
            }
            current_frame.end = time_usec
            current_frame = current_frame.caller
        }
    }
    return ret
}
