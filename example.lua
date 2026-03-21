local nft = require("nftables")
local ctx <close> = nft.new()

assert(ctx:cmd([[
add table bridge teste
add chain bridge teste input { type filter hook input priority 0; }
add rule bridge teste input tcp dport 443 counter
]]))

print(ctx:cmd("list table bridge teste"))

