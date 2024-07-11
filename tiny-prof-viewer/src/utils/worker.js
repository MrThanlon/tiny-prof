import { parse } from './parse'

onmessage = (e) => {
  const { profile, symbolMap } = e.data
  try {
    postMessage({
      ok: true,
      data: parse(profile, symbolMap)
    })
  } catch (e) {
    postMessage({
      ok: false,
      error: e
    })
  }
}
