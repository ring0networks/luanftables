local nft = require("nftables")
local ctx <close> = nft.new()

-- create table and chain
assert(ctx:cmd([[
add table bridge teste_json
add chain bridge teste_json filter { type filter hook forward priority 0; }
add rule bridge teste_json filter tcp dport 443 counter
]]))

-- list as JSON
ctx:output("json", true)
local json = assert(ctx:cmd("list table bridge teste_json"))
print("=== JSON output ===")
print(json)

-- delete and recreate from JSON input
ctx:output("json", false)
ctx:cmd("delete table bridge teste_json")

ctx:input("json", true)
assert(ctx:cmd(json))
ctx:input("json", false)

-- verify it's back
local out = assert(ctx:cmd("list table bridge teste_json"))
print("\n=== after JSON roundtrip ===")
print(out)

-- cleanup
ctx:cmd("delete table bridge teste_json")

