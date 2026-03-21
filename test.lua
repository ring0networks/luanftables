--
-- SPDX-FileCopyrightText: (c) 2026 Ring Zero Desenvolvimento de Software LTDA
-- SPDX-License-Identifier: MIT
--

local nft = require("nftables")

local function test(name, fn)
	local ok, err = pcall(fn)
	if ok then
		print(string.format("  PASS  %s", name))
	else
		print(string.format("  FAIL  %s: %s", name, err))
		os.exit(1)
	end
end

-- test: create and list table
test("create and list table", function()
	local ctx <close> = nft.new()
	assert(ctx:cmd("add table bridge test_lua"))
	local out = ctx:cmd("list table bridge test_lua")
	assert(out and out:find("test_lua"), "table not found in output")
	ctx:cmd("delete table bridge test_lua")
end)

-- test: error on nonexistent table
test("error on nonexistent table", function()
	local ctx <close> = nft.new()
	local ok, err = ctx:cmd("delete table bridge nonexistent_xxx")
	assert(ok == nil, "expected nil on error")
	assert(err and #err > 0, "expected error message")
end)

-- test: json output flag
test("json output flag", function()
	local ctx <close> = nft.new()
	assert(ctx:output("json") == false)
	ctx:output("json", true)
	assert(ctx:output("json") == true)
	assert(ctx:cmd("add table bridge test_json"))
	local out = ctx:cmd("list table bridge test_json")
	assert(out and out:find('"family"'), "expected JSON output")
	ctx:output("json", false)
	assert(ctx:output("json") == false)
	ctx:cmd("delete table bridge test_json")
end)

-- test: input flags
test("input flags", function()
	local ctx <close> = nft.new()
	assert(ctx:input("nodns") == false)
	ctx:input("nodns", true)
	assert(ctx:input("nodns") == true)
	ctx:input("nodns", false)
	assert(ctx:input("nodns") == false)
end)

-- test: dryrun flag
test("dryrun flag", function()
	local ctx <close> = nft.new()
	assert(ctx:dryrun() == false)
	ctx:dryrun(true)
	assert(ctx:dryrun() == true)
	assert(ctx:cmd("add table bridge test_dryrun"))
	ctx:dryrun(false)
end)

-- test: optimize flag
test("optimize flag", function()
	local ctx <close> = nft.new()
	assert(ctx:optimize() == false)
	ctx:optimize(true)
	assert(ctx:optimize() == true)
	ctx:optimize(false)
end)

-- test: run file
test("run file", function()
	local ctx <close> = nft.new()
	local tmpfile = os.tmpname()
	local f <close> = io.open(tmpfile, "w")
	f:write("add table bridge test_file\n")
	f:write("delete table bridge test_file\n")
	f:close()
	assert(ctx:run(tmpfile))
	os.remove(tmpfile)
end)

-- test: run nonexistent file
test("run nonexistent file", function()
	local ctx <close> = nft.new()
	local ok, err = ctx:run("/tmp/nonexistent_nft_file_xxx")
	assert(ok == nil, "expected nil on error")
	assert(err and #err > 0, "expected error message")
end)

-- test: unknown flag error
test("unknown flag error", function()
	local ctx <close> = nft.new()
	local ok, err = pcall(ctx.output, ctx, "invalid_flag", true)
	assert(not ok, "expected error for unknown flag")
end)

-- test: close is idempotent
test("close is idempotent", function()
	local ctx = nft.new()
	ctx:close()
	ctx:close()
end)

-- test: use after close errors
test("use after close errors", function()
	local ctx = nft.new()
	ctx:close()
	local ok, err = pcall(ctx.cmd, ctx, "list ruleset")
	assert(not ok, "expected error after close")
end)

-- test: batch commands
test("batch commands", function()
	local ctx <close> = nft.new()
	assert(ctx:cmd("add table bridge test_batch\nadd chain bridge test_batch input"))
	local out = ctx:cmd("list table bridge test_batch")
	assert(out and out:find("input"), "chain not found")
	ctx:cmd("delete table bridge test_batch")
end)

-- test: multiple output flags
test("multiple output flags", function()
	local ctx <close> = nft.new()
	ctx:output("handle", true)
	ctx:output("stateless", true)
	assert(ctx:output("handle") == true)
	assert(ctx:output("stateless") == true)
	ctx:output("handle", false)
	assert(ctx:output("handle") == false)
	assert(ctx:output("stateless") == true)
end)

-- test: flag constants exported
test("flag constants exported", function()
	assert(nft.output.json == 0x010)
	assert(nft.input.nodns == 0x1)
	assert(nft.debug.scanner == 0x01)
end)

print("OK")

