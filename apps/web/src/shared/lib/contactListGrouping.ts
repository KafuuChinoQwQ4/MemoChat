const CONTACT_INITIALS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("")

const PINYIN_BOUNDARIES: ReadonlyArray<readonly [string, string]> = [
  ["A", "阿"], ["B", "芭"], ["C", "擦"], ["D", "搭"], ["E", "蛾"],
  ["F", "发"], ["G", "噶"], ["H", "哈"], ["J", "击"], ["K", "喀"],
  ["L", "垃"], ["M", "妈"], ["N", "拿"], ["O", "哦"], ["P", "啪"],
  ["Q", "期"], ["R", "然"], ["S", "撒"], ["T", "塌"], ["W", "挖"],
  ["X", "昔"], ["Y", "压"], ["Z", "匝"],
]

const PINYIN_COLLATOR = new Intl.Collator("zh-CN-u-co-pinyin", {
  usage: "sort",
  sensitivity: "base",
  numeric: true,
})

export interface ContactInitialGroup<T> {
  initial: string
  items: T[]
}

export function contactListInitial(name: string): string {
  const first = Array.from(name.trim())[0]
  if (!first) return "#"
  if (/^[A-Za-z]$/.test(first)) return first.toUpperCase()
  if (!/^\p{Script=Han}$/u.test(first)) return "#"

  let initial = "#"
  for (const [letter, boundary] of PINYIN_BOUNDARIES) {
    if (PINYIN_COLLATOR.compare(first, boundary) < 0) break
    initial = letter
  }
  return initial
}

export function groupContactsByInitial<T>(
  items: readonly T[],
  getName: (item: T) => string,
): ContactInitialGroup<T>[] {
  const grouped = new Map<string, T[]>()
  for (const item of items) {
    const initial = contactListInitial(getName(item))
    const group = grouped.get(initial)
    if (group) group.push(item)
    else grouped.set(initial, [item])
  }

  const initialOrder = [...CONTACT_INITIALS, "#"]
  return initialOrder.flatMap((initial) => {
    const group = grouped.get(initial)
    if (!group) return []
    return [{
      initial,
      items: group.slice().sort((left, right) => PINYIN_COLLATOR.compare(getName(left), getName(right))),
    }]
  })
}
