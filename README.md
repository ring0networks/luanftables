# luanftables

Lua binding for [libnftables](https://wiki.nftables.org/wiki-nftables/index.php/Building_and_installing_nftables_from_sources) — programmatic access to nftables from Lua.

## Build

Requires `libnftables-dev` and Lua (>= 5.4) headers.

```
sudo apt install libnftables-dev lua5.4 liblua5.4-dev
make
sudo make install
```

The build auto-detects the Lua version via `pkg-config` (prefers 5.5, falls back to 5.4).
To override: `make LUA_VERSION=5.4`.

## Usage

```lua
local nft = require("nftables")
local ctx <close> = nft.new()

-- run nftables commands
local output, err = ctx:cmd("add table bridge dome")
local output, err = ctx:cmd("list ruleset")

-- run commands from file
local output, err = ctx:run("rules.nft")

-- multiple commands (atomic)
ctx:cmd("add table bridge dome\nadd chain bridge dome filter")

-- close (also via <close> or __gc)
ctx:close()
```

## Error handling

`ctx:cmd()` and `ctx:run()` return the output string on success, or `nil` +
error message on failure.

```lua
local output, err = ctx:cmd("delete table bridge nonexistent")
if not output then
    print("error: " .. err)
end
```

## Flags

Each flag group has its own method. Call with a value to set, without to get.

```lua
ctx:output("json", true)       -- JSON output
ctx:output("handle", true)     -- include handles in output
ctx:output("stateless", true)  -- omit counters/quotas
ctx:output("terse", true)      -- omit set element contents
ctx:output("echo", true)       -- print changes after commit

print(ctx:output("json"))      -- true

ctx:input("nodns", true)       -- no DNS lookups during parsing
ctx:input("json", true)        -- accept JSON input

ctx:dryrun(true)               -- validate without applying
ctx:optimize(true)             -- optimize ruleset (nft -o)
```

| Method | Flags |
|--------|-------|
| `ctx:output(name [, bool])` | `reversedns`, `service`, `stateless`, `handle`, `json`, `echo`, `guid`, `numeric_proto`, `numeric_prio`, `numeric_symbol`, `numeric_time`, `terse` |
| `ctx:input(name [, bool])` | `nodns`, `json` |
| `ctx:debug(name [, bool])` | `scanner`, `parser`, `evaluation`, `netlink`, `mnl`, `proto_ctx`, `segtree` |
| `ctx:dryrun([bool])` | validate without applying |
| `ctx:optimize([bool])` | optimize ruleset |

Flag constants are also exported as `nft.output`, `nft.input`, `nft.debug` tables.

## JSON

```lua
local nft = require("nftables")
local ctx <close> = nft.new()

-- create a table and chain
ctx:cmd("add table bridge dome")
ctx:cmd("add chain bridge dome filter { type filter hook forward priority 0; }")

-- list ruleset as JSON
ctx:output("json", true)
local json = ctx:cmd("list table bridge dome")
print(json)

-- submit JSON input
ctx:input("json", true)
ctx:cmd([[{"nftables": [
    {"add": {"rule": {
        "family": "bridge", "table": "dome", "chain": "filter",
        "expr": [
            {"match": {"left": {"payload": {"protocol": "tcp", "field": "dport"}},
                        "op": "==", "right": 443}},
            {"counter": null}
        ]
    }}}
]}]])
```

## Variables and includes

```lua
-- variables (accessible as $key in nft scripts)
ctx:var("IFACE=br-lan")
ctx:clear_vars()

-- include paths (for nft include directives)
ctx:include("/etc/nft/")
ctx:clear_includes()
```

## [API](https://ring0networks.github.io/luanftables/)

| Method | Description |
|--------|-------------|
| `nft.new()` | Create a new nftables context |
| `ctx:cmd(str)` | Execute nftables command(s) from string |
| `ctx:run(file)` | Execute nftables commands from file |
| `ctx:output(name [, bool])` | Get/set output flag |
| `ctx:input(name [, bool])` | Get/set input flag |
| `ctx:debug(name [, bool])` | Get/set debug flag |
| `ctx:dryrun([bool])` | Get/set dry-run mode |
| `ctx:optimize([bool])` | Get/set optimization |
| `ctx:var(kv)` | Define a variable (`"key=value"`) |
| `ctx:clear_vars()` | Remove all variables |
| `ctx:include(path)` | Add an include search path |
| `ctx:clear_includes()` | Remove all include paths |
| `ctx:close()` | Free the context (idempotent) |

## Requirements

- Lua >= 5.4
- libnftables (runtime)
- libnftables-dev (build)
- Root or `CAP_NET_ADMIN` to execute nftables commands

## License

MIT

