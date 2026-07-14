import { describe, expect, it } from "vitest"
import { contactListInitial, groupContactsByInitial } from "./contactListGrouping"

describe("contactListGrouping", () => {
  it.each([
    ["alice", "A"],
    ["Bob", "B"],
    ["陈晨", "C"],
    ["群聊", "Q"],
    ["张三", "Z"],
    ["123群", "#"],
    ["@好友", "#"],
    ["😀伙伴", "#"],
    ["", "#"],
  ])("maps %s to %s", (name, initial) => {
    expect(contactListInitial(name)).toBe(initial)
  })

  it("groups A-Z first and puts special-character names in the final # section", () => {
    const grouped = groupContactsByInitial(
      ["张三", "alice", "@置顶", "白雪", "Bob", "123群", "陈晨"],
      (name) => name,
    )

    expect(grouped.map((group) => group.initial)).toEqual(["A", "B", "C", "Z", "#"])
    expect(grouped.find((group) => group.initial === "B")?.items).toEqual(["白雪", "Bob"])
    expect(grouped[grouped.length - 1]?.items).toEqual(["@置顶", "123群"])
  })
})
