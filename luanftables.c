/*
 * SPDX-FileCopyrightText: (c) 2026 Ring Zero Desenvolvimento de Software LTDA
 * SPDX-License-Identifier: MIT
 */

/***
 * Lua binding for libnftables.
 * @module nftables
 */

#include <stdlib.h>
#include <string.h>

#include <nftables/libnftables.h>

#include <lua.h>
#include <lauxlib.h>

#define LUANFT_CONTEXT	"nftables.context"

typedef struct {
	const char *name;
	unsigned int flag;
} luanft_flag_t;

static const luanft_flag_t luanft_output_flags[] = {
	{"reversedns",		NFT_CTX_OUTPUT_REVERSEDNS},
	{"service",		NFT_CTX_OUTPUT_SERVICE},
	{"stateless",		NFT_CTX_OUTPUT_STATELESS},
	{"handle",		NFT_CTX_OUTPUT_HANDLE},
	{"json",		NFT_CTX_OUTPUT_JSON},
	{"echo",		NFT_CTX_OUTPUT_ECHO},
	{"guid",		NFT_CTX_OUTPUT_GUID},
	{"numeric_proto",	NFT_CTX_OUTPUT_NUMERIC_PROTO},
	{"numeric_prio",	NFT_CTX_OUTPUT_NUMERIC_PRIO},
	{"numeric_symbol",	NFT_CTX_OUTPUT_NUMERIC_SYMBOL},
	{"numeric_time",	NFT_CTX_OUTPUT_NUMERIC_TIME},
	{"terse",		NFT_CTX_OUTPUT_TERSE},
	{NULL, 0}
};

static const luanft_flag_t luanft_input_flags[] = {
	{"nodns",	NFT_CTX_INPUT_NO_DNS},
	{"json",	NFT_CTX_INPUT_JSON},
	{NULL, 0}
};

static const luanft_flag_t luanft_debug_flags[] = {
	{"scanner",	NFT_DEBUG_SCANNER},
	{"parser",	NFT_DEBUG_PARSER},
	{"evaluation",	NFT_DEBUG_EVALUATION},
	{"netlink",	NFT_DEBUG_NETLINK},
	{"mnl",		NFT_DEBUG_MNL},
	{"proto_ctx",	NFT_DEBUG_PROTO_CTX},
	{"segtree",	NFT_DEBUG_SEGTREE},
	{NULL, 0}
};

static inline const luanft_flag_t *luanft_find_flag(const luanft_flag_t *flags, const char *name)
{
	for (const luanft_flag_t *f = flags; f->name != NULL; f++)
		if (strcmp(name, f->name) == 0)
			return f;
	return NULL;
}

static inline struct nft_ctx **luanft_check(lua_State *L, int ix)
{
	struct nft_ctx **ctx = (struct nft_ctx **)luaL_testudata(L, ix, LUANFT_CONTEXT);
	luaL_argcheck(L, ctx != NULL, ix, "nftables context expected");
	return ctx;
}

static inline struct nft_ctx *luanft_checkctx(lua_State *L, int ix)
{
	struct nft_ctx **ctx = luanft_check(L, ix);
	luaL_argcheck(L, *ctx != NULL, ix, "nftables context already closed");
	return *ctx;
}

static inline void luanft_pushstring(lua_State *L, const char *s, const char *fallback)
{
	const char *str = (s != NULL && s[0] != '\0') ? s : fallback;
	lua_pushstring(L, str);
}

static int luanft_result(lua_State *L, struct nft_ctx *ctx, int rc)
{
	if (rc != 0) {
		lua_pushnil(L);
		luanft_pushstring(L, nft_ctx_get_error_buffer(ctx), "nftables command failed");
		return 2;
	}
	luanft_pushstring(L, nft_ctx_get_output_buffer(ctx), "");
	return 1;
}

/***
 * Create a new nftables context.
 * Output and error buffering are enabled automatically.
 * @function new
 * @treturn context nftables context (supports `<close>`)
 * @usage
 * local nft = require("nftables")
 * local ctx <close> = nftables.new()
 */
static int luanft_new(lua_State *L)
{
	struct nft_ctx *ctx = nft_ctx_new(NFT_CTX_DEFAULT);
	if (ctx == NULL)
		return luaL_error(L, "failed to create nftables context");

	nft_ctx_buffer_output(ctx);
	nft_ctx_buffer_error(ctx);

	struct nft_ctx **ud = (struct nft_ctx **)lua_newuserdatauv(L, sizeof(*ud), 0);
	*ud = ctx;

	luaL_setmetatable(L, LUANFT_CONTEXT);
	return 1;
}

/***
 * Execute nftables command(s) from a string.
 * Multiple commands can be separated by newlines and are applied atomically.
 * @function cmd
 * @tparam string command nftables command(s)
 * @treturn string output on success
 * @treturn[2] nil
 * @treturn[2] string error message
 * @usage
 * local out, err = ctx:cmd("add table bridge dome")
 * ctx:cmd("add table bridge dome\nadd chain bridge dome filter")
 */
static int luanft_cmd(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	const char *cmd = luaL_checkstring(L, 2);
	int rc = nft_run_cmd_from_buffer(ctx, cmd);
	return luanft_result(L, ctx, rc);
}

/***
 * Execute nftables commands from a file.
 * @function run
 * @tparam string filename path to nftables script
 * @treturn string output on success
 * @treturn[2] nil
 * @treturn[2] string error message
 */
static int luanft_run(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	const char *filename = luaL_checkstring(L, 2);
	int rc = nft_run_cmd_from_filename(ctx, filename);
	return luanft_result(L, ctx, rc);
}

static int luanft_flagop(lua_State *L, const luanft_flag_t *flags, const char *group, unsigned int current, unsigned int *updated)
{
	const char *name = luaL_checkstring(L, 2);

	const luanft_flag_t *f = luanft_find_flag(flags, name);
	if (f == NULL)
		return luaL_error(L, "unknown %s flag: %s", group, name);

	if (lua_gettop(L) == 2) {
		lua_pushboolean(L, current & f->flag);
		return 1;
	}

	*updated = lua_toboolean(L, 3) ? current | f->flag : current & ~f->flag;
	return 0;
}

/***
 * Get or set an output flag.
 * Flags: `reversedns`, `service`, `stateless`, `handle`, `json`, `echo`,
 * `guid`, `numeric_proto`, `numeric_prio`, `numeric_symbol`, `numeric_time`,
 * `terse`.
 * @function output
 * @tparam string name flag name
 * @tparam[opt] bool value set flag if provided
 * @treturn bool current value (when getting)
 * @usage
 * ctx:output("json", true)
 * print(ctx:output("json"))  -- true
 */
static int luanft_output(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	unsigned int flags = nft_ctx_output_get_flags(ctx);
	int rc = luanft_flagop(L, luanft_output_flags, "output", flags, &flags);
	if (rc == 0)
		nft_ctx_output_set_flags(ctx, flags);
	return rc;
}

/***
 * Get or set an input flag.
 * Flags: `nodns`, `json`.
 * @function input
 * @tparam string name flag name
 * @tparam[opt] bool value set flag if provided
 * @treturn bool current value (when getting)
 * @usage
 * ctx:input("nodns", true)
 */
static int luanft_input(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	unsigned int flags = nft_ctx_input_get_flags(ctx);
	int rc = luanft_flagop(L, luanft_input_flags, "input", flags, &flags);
	if (rc == 0)
		nft_ctx_input_set_flags(ctx, flags);
	return rc;
}

/***
 * Get or set a debug flag.
 * Flags: `scanner`, `parser`, `evaluation`, `netlink`, `mnl`, `proto_ctx`,
 * `segtree`.
 * @function debug
 * @tparam string name flag name
 * @tparam[opt] bool value set flag if provided
 * @treturn bool current value (when getting)
 */
static int luanft_debug(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	unsigned int flags = nft_ctx_output_get_debug(ctx);
	int rc = luanft_flagop(L, luanft_debug_flags, "debug", flags, &flags);
	if (rc == 0)
		nft_ctx_output_set_debug(ctx, flags);
	return rc;
}

/***
 * Get or set dry-run mode.
 * When enabled, commands are parsed and validated but not applied.
 * @function dryrun
 * @tparam[opt] bool value set mode if provided
 * @treturn bool current value (when getting)
 */
static int luanft_dryrun(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);

	if (lua_gettop(L) == 1) {
		lua_pushboolean(L, nft_ctx_get_dry_run(ctx));
		return 1;
	}

	nft_ctx_set_dry_run(ctx, lua_toboolean(L, 2));
	return 0;
}

/***
 * Get or set ruleset optimization.
 * When enabled, nftables performs a two-pass operation: validates in dry-run,
 * then optimizes (e.g., collapsing linear rules into sets) before committing.
 * @function optimize
 * @tparam[opt] bool value set mode if provided
 * @treturn bool current value (when getting)
 */
static int luanft_optimize(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);

	if (lua_gettop(L) == 1) {
		lua_pushboolean(L, nft_ctx_get_optimize(ctx) & NFT_OPTIMIZE_ENABLED);
		return 1;
	}

	nft_ctx_set_optimize(ctx, lua_toboolean(L, 2) ? NFT_OPTIMIZE_ENABLED : 0);
	return 0;
}

/***
 * Add an include search path for nft scripts.
 * @function include
 * @tparam string path directory path
 */
static int luanft_include(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	const char *path = luaL_checkstring(L, 2);
	if (nft_ctx_add_include_path(ctx, path) != 0)
		return luaL_error(L, "failed to add include path: %s", path);
	return 0;
}

/***
 * Remove all include search paths.
 * @function clear_includes
 */
static int luanft_clear_includes(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	nft_ctx_clear_include_paths(ctx);
	return 0;
}

/***
 * Define a variable accessible as `$key` in nft scripts.
 * @function var
 * @tparam string kv variable in `"key=value"` format
 * @usage
 * ctx:var("IFACE=br-lan")
 */
static int luanft_var(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	const char *var = luaL_checkstring(L, 2);
	if (nft_ctx_add_var(ctx, var) != 0)
		return luaL_error(L, "failed to add variable: %s", var);
	return 0;
}

/***
 * Remove all defined variables.
 * @function clear_vars
 */
static int luanft_clear_vars(lua_State *L)
{
	struct nft_ctx *ctx = luanft_checkctx(L, 1);
	nft_ctx_clear_vars(ctx);
	return 0;
}

/***
 * Close the nftables context and free resources.
 * Idempotent — safe to call multiple times.
 * Also called automatically via `__gc` and `__close`.
 * @function close
 */
static int luanft_close(lua_State *L)
{
	struct nft_ctx **ctx = luanft_check(L, 1);
	if (*ctx != NULL) {
		nft_ctx_free(*ctx);
		*ctx = NULL;
	}
	return 0;
}

static const luaL_Reg luanft_methods[] = {
	{"cmd",			luanft_cmd},
	{"run",			luanft_run},
	{"output",		luanft_output},
	{"input",		luanft_input},
	{"debug",		luanft_debug},
	{"dryrun",		luanft_dryrun},
	{"optimize",		luanft_optimize},
	{"include",		luanft_include},
	{"clear_includes",	luanft_clear_includes},
	{"var",			luanft_var},
	{"clear_vars",		luanft_clear_vars},
	{"close",		luanft_close},
	{"__gc",		luanft_close},
	{"__close",		luanft_close},
	{NULL, NULL}
};

static const luaL_Reg luanft_lib[] = {
	{"new",	luanft_new},
	{NULL, NULL}
};

static void luanft_pushflags(lua_State *L, const char *name, const luanft_flag_t *flags)
{
	lua_newtable(L);
	for (const luanft_flag_t *f = flags; f->name != NULL; f++) {
		lua_pushinteger(L, f->flag);
		lua_setfield(L, -2, f->name);
	}
	lua_setfield(L, -2, name);
}

int luaopen_nftables(lua_State *L)
{
	luaL_newmetatable(L, LUANFT_CONTEXT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, luanft_methods, 0);
	lua_pop(L, 1);

	luaL_newlib(L, luanft_lib);

	luanft_pushflags(L, "output", luanft_output_flags);
	luanft_pushflags(L, "input", luanft_input_flags);
	luanft_pushflags(L, "debug", luanft_debug_flags);

	return 1;
}

