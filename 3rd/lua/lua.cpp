#include <lua.hpp>
#include <windows.h>
#include <vector>

/*
该文件的作用是为了处理 动态加载lua时， 宿主存在多个lua模块 并且不同名字混乱的问题。
*/


HMODULE g_lua_module = nullptr;


struct StackFuncInfo
{
	uintptr_t address;
	HMODULE module;
};

inline HMODULE GetModuleFromAddr(PVOID p)
{
	MEMORY_BASIC_INFORMATION m = { 0 };
	VirtualQuery(p, &m, sizeof(MEMORY_BASIC_INFORMATION));
	return (HMODULE)m.AllocationBase;
}

void track_stack(std::vector<StackFuncInfo>& list)
{
	static const int MAX_STACK_FRAMES = 12;

	void* pStack[MAX_STACK_FRAMES];

	WORD frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pStack, NULL);

	for (WORD i = 0; i < frames; ++i) 
	{

		uintptr_t address = (uintptr_t)(pStack[i]);
		StackFuncInfo info;
		info.address = address;
		info.module = GetModuleFromAddr((void*)address);

		list.push_back(info);
	}
}
//初始化当前模块的时候 从调用堆栈里拿到宿主的handle
extern "C" void init_lua_module()
{
	std::vector<StackFuncInfo> list;

	track_stack(list);

	for (auto& info: list)
	{
		if (info.module && GetProcAddress(info.module, "lua_version"))
		{
			g_lua_module = info.module;
			break;
		}
	}
}

HMODULE get_module()
{
	if (g_lua_module) {
		return g_lua_module;
	}
		
	HMODULE handle;
	
	const char* list[] = {
		"luacore.dll",
		"lua53.dll",
		"lua.dll",
	};
	
	for (int i = 0; i < sizeof(list) / sizeof(const char*); i++)
	{
		handle = GetModuleHandleA(list[i]);
		if (handle && GetProcAddress(handle, "lua_version")) return handle;
	}
	for (int i = 0; i < sizeof(list) / sizeof(const char*); i++)
	{
		handle = LoadLibraryA(list[i]);
		if (handle && GetProcAddress(handle, "lua_version")) return handle;
	}
	return GetModuleHandleA(nullptr);
}

uintptr_t get_proaddress(const char* name)
{
	static HMODULE handle = get_module();
	return (uintptr_t)GetProcAddress(handle, name);
}
lua_State* lua_newstate(lua_Alloc f, void* ud) {
	static auto ptr = ((decltype(lua_newstate)*)get_proaddress("lua_newstate"));
	return ptr(f, ud);
}
lua_State* lua_newstate2(lua_Alloc f, void* ud, unsigned int seed) {
	static auto ptr = ((decltype(lua_newstate2)*)get_proaddress("lua_newstate2"));
	return ptr(f, ud, seed);
}
void       lua_setgchash(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_setgchash)*)get_proaddress("lua_setgchash"));
	ptr(L, idx);
}
void       lua_close(lua_State* L) {
	static auto ptr = ((decltype(lua_close)*)get_proaddress("lua_close"));
	ptr(L);
}
lua_State* lua_newthread(lua_State* L) {
	static auto ptr = ((decltype(lua_newthread)*)get_proaddress("lua_newthread"));
	return ptr(L);
}
lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf) {
	static auto ptr = ((decltype(lua_atpanic)*)get_proaddress("lua_atpanic"));
	return ptr(L, panicf);
}
const lua_Number* lua_version(lua_State* L) {
	static auto ptr = ((decltype(lua_version)*)get_proaddress("lua_version"));
	return ptr(L);
}
int   lua_absindex(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_absindex)*)get_proaddress("lua_absindex"));
	return ptr(L, idx);
}
int   lua_gettop(lua_State* L) {
	static auto ptr = ((decltype(lua_gettop)*)get_proaddress("lua_gettop"));
	return ptr(L);
}
void  lua_settop(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_settop)*)get_proaddress("lua_settop"));
	ptr(L, idx);
}
void  lua_pushvalue(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_pushvalue)*)get_proaddress("lua_pushvalue"));
	ptr(L, idx);
}
void  lua_rotate(lua_State* L, int idx, int n) {
	static auto ptr = ((decltype(lua_rotate)*)get_proaddress("lua_rotate"));
	ptr(L, idx, n);
}
void  lua_copy(lua_State* L, int fromidx, int toidx) {
	static auto ptr = ((decltype(lua_copy)*)get_proaddress("lua_copy"));
	ptr(L, fromidx, toidx);
}
int   lua_checkstack(lua_State* L, int n) {
	static auto ptr = ((decltype(lua_checkstack)*)get_proaddress("lua_checkstack"));
	return ptr(L, n);
}
void  lua_xmove(lua_State* from, lua_State* to, int n) {
	static auto ptr = ((decltype(lua_xmove)*)get_proaddress("lua_xmove"));
	ptr(from, to, n);
}
int             lua_isnumber(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_isnumber)*)get_proaddress("lua_isnumber"));
	return ptr(L, idx);
}
int             lua_isstring(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_isstring)*)get_proaddress("lua_isstring"));
	return ptr(L, idx);
}
int             lua_iscfunction(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_iscfunction)*)get_proaddress("lua_iscfunction"));
	return ptr(L, idx);
}
int             lua_isinteger(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_isinteger)*)get_proaddress("lua_isinteger"));
	return ptr(L, idx);
}
int             lua_isuserdata(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_isuserdata)*)get_proaddress("lua_isuserdata"));
	return ptr(L, idx);
}
int             lua_type(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_type)*)get_proaddress("lua_type"));
	return ptr(L, idx);
}
const char* lua_typename(lua_State* L, int tp) {
	static auto ptr = ((decltype(lua_typename)*)get_proaddress("lua_typename"));
	return ptr(L, tp);
}
lua_Number      lua_tonumberx(lua_State* L, int idx, int* isnum) {
	static auto ptr = ((decltype(lua_tonumberx)*)get_proaddress("lua_tonumberx"));
	return ptr(L, idx, isnum);
}
lua_Integer     lua_tointegerx(lua_State* L, int idx, int* isnum) {
	static auto ptr = ((decltype(lua_tointegerx)*)get_proaddress("lua_tointegerx"));
	return ptr(L, idx, isnum);
}
int             lua_toboolean(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_toboolean)*)get_proaddress("lua_toboolean"));
	return ptr(L, idx);
}
const char* lua_tolstring(lua_State* L, int idx, size_t* len) {
	static auto ptr = ((decltype(lua_tolstring)*)get_proaddress("lua_tolstring"));
	return ptr(L, idx, len);
}
size_t          lua_rawlen(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_rawlen)*)get_proaddress("lua_rawlen"));
	return ptr(L, idx);
}
lua_CFunction   lua_tocfunction(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_tocfunction)*)get_proaddress("lua_tocfunction"));
	return ptr(L, idx);
}
void* lua_touserdata(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_touserdata)*)get_proaddress("lua_touserdata"));
	return ptr(L, idx);
}
lua_State* lua_tothread(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_tothread)*)get_proaddress("lua_tothread"));
	return ptr(L, idx);
}
const void* lua_topointer(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_topointer)*)get_proaddress("lua_topointer"));
	return ptr(L, idx);
}
void  lua_arith(lua_State* L, int op) {
	static auto ptr = ((decltype(lua_arith)*)get_proaddress("lua_arith"));
	ptr(L, op);
}
int   lua_rawequal(lua_State* L, int idx1, int idx2) {
	static auto ptr = ((decltype(lua_rawequal)*)get_proaddress("lua_rawequal"));
	return ptr(L, idx1, idx2);
}
int   lua_compare(lua_State* L, int idx1, int idx2, int op) {
	static auto ptr = ((decltype(lua_compare)*)get_proaddress("lua_compare"));
	return ptr(L, idx1, idx2, op);
}
void        lua_pushnil(lua_State* L) {
	static auto ptr = ((decltype(lua_pushnil)*)get_proaddress("lua_pushnil"));
	ptr(L);
}
void        lua_pushnumber(lua_State* L, lua_Number n) {
	static auto ptr = ((decltype(lua_pushnumber)*)get_proaddress("lua_pushnumber"));
	ptr(L, n);
}
void        lua_pushinteger(lua_State* L, lua_Integer n) {
	static auto ptr = ((decltype(lua_pushinteger)*)get_proaddress("lua_pushinteger"));
	ptr(L, n);
}
const char* lua_pushlstring(lua_State* L, const char* s, size_t len) {
	static auto ptr = ((decltype(lua_pushlstring)*)get_proaddress("lua_pushlstring"));
	return ptr(L, s, len);
}
const char* lua_pushstring(lua_State* L, const char* s) {
	static auto ptr = ((decltype(lua_pushstring)*)get_proaddress("lua_pushstring"));
	return ptr(L, s);
}
const char* lua_pushvfstring(lua_State* L, const char* fmt,
	va_list argp) {
	static auto ptr = ((decltype(lua_pushvfstring)*)get_proaddress("lua_pushvfstring"));
	return ptr(L, fmt, argp);
}
const char* lua_pushfstring(lua_State* L, const char* fmt, ...) {
	static auto ptr = ((decltype(lua_pushfstring)*)get_proaddress("lua_pushfstring"));
	return ptr(L, fmt);
}
void  lua_pushcclosure(lua_State* L, lua_CFunction fn, int n) {
	static auto ptr = ((decltype(lua_pushcclosure)*)get_proaddress("lua_pushcclosure"));
	ptr(L, fn, n);
}
void  lua_pushboolean(lua_State* L, int b) {
	static auto ptr = ((decltype(lua_pushboolean)*)get_proaddress("lua_pushboolean"));
	ptr(L, b);
}
void  lua_pushlightuserdata(lua_State* L, void* p) {
	static auto ptr = ((decltype(lua_pushlightuserdata)*)get_proaddress("lua_pushlightuserdata"));
	ptr(L, p);
}
int   lua_pushthread(lua_State* L) {
	static auto ptr = ((decltype(lua_pushthread)*)get_proaddress("lua_pushthread"));
	return ptr(L);
}
int lua_getglobal(lua_State* L, const char* name) {
	static auto ptr = ((decltype(lua_getglobal)*)get_proaddress("lua_getglobal"));
	return ptr(L, name);
}
int lua_gettable(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_gettable)*)get_proaddress("lua_gettable"));
	return ptr(L, idx);
}
int lua_getfield(lua_State* L, int idx, const char* k) {
	static auto ptr = ((decltype(lua_getfield)*)get_proaddress("lua_getfield"));
	return ptr(L, idx, k);
}
int lua_geti(lua_State* L, int idx, lua_Integer n) {
	static auto ptr = ((decltype(lua_geti)*)get_proaddress("lua_geti"));
	return ptr(L, idx, n);
}
int lua_rawget(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_rawget)*)get_proaddress("lua_rawget"));
	return ptr(L, idx);
}
int lua_rawgeti(lua_State* L, int idx, lua_Integer n) {
	static auto ptr = ((decltype(lua_rawgeti)*)get_proaddress("lua_rawgeti"));
	return ptr(L, idx, n);
}
int lua_rawgetp(lua_State* L, int idx, const void* p) {
	static auto ptr = ((decltype(lua_rawgetp)*)get_proaddress("lua_rawgetp"));
	return ptr(L, idx, p);
}
void  lua_createtable(lua_State* L, int narr, int nrec) {
	static auto ptr = ((decltype(lua_createtable)*)get_proaddress("lua_createtable"));
	ptr(L, narr, nrec);
}
void* lua_newuserdata(lua_State* L, size_t sz) {
	static auto ptr = ((decltype(lua_newuserdata)*)get_proaddress("lua_newuserdata"));
	return ptr(L, sz);
}
int   lua_getmetatable(lua_State* L, int objindex) {
	static auto ptr = ((decltype(lua_getmetatable)*)get_proaddress("lua_getmetatable"));
	return ptr(L, objindex);
}
int  lua_getuservalue(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_getuservalue)*)get_proaddress("lua_getuservalue"));
	return ptr(L, idx);
}
void  lua_setglobal(lua_State* L, const char* name) {
	static auto ptr = ((decltype(lua_setglobal)*)get_proaddress("lua_setglobal"));
	ptr(L, name);
}
void  lua_settable(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_settable)*)get_proaddress("lua_settable"));
	ptr(L, idx);
}
void  lua_setfield(lua_State* L, int idx, const char* k) {
	static auto ptr = ((decltype(lua_setfield)*)get_proaddress("lua_setfield"));
	ptr(L, idx, k);
}
void  lua_seti(lua_State* L, int idx, lua_Integer n) {
	static auto ptr = ((decltype(lua_seti)*)get_proaddress("lua_seti"));
	ptr(L, idx, n);
}
void  lua_rawset(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_rawset)*)get_proaddress("lua_rawset"));
	ptr(L, idx);
}
void  lua_rawseti(lua_State* L, int idx, lua_Integer n) {
	static auto ptr = ((decltype(lua_rawseti)*)get_proaddress("lua_rawseti"));
	ptr(L, idx, n);
}
void  lua_rawsetp(lua_State* L, int idx, const void* p) {
	static auto ptr = ((decltype(lua_rawsetp)*)get_proaddress("lua_rawsetp"));
	ptr(L, idx, p);
}
int   lua_setmetatable(lua_State* L, int objindex) {
	static auto ptr = ((decltype(lua_setmetatable)*)get_proaddress("lua_setmetatable"));
	return ptr(L, objindex);
}
void  lua_setuservalue(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_setuservalue)*)get_proaddress("lua_setuservalue"));
	ptr(L, idx);
}
void  lua_callk(lua_State* L, int nargs, int nresults,
	lua_KContext ctx, lua_KFunction k) {
	static auto ptr = ((decltype(lua_callk)*)get_proaddress("lua_callk"));
	ptr(L, nargs, nresults, ctx, k);
}
int   lua_pcallk(lua_State* L, int nargs, int nresults, int errfunc,
	lua_KContext ctx, lua_KFunction k) {
	static auto ptr = ((decltype(lua_pcallk)*)get_proaddress("lua_pcallk"));
	return ptr(L, nargs, nresults, errfunc, ctx, k);
}
int   lua_load(lua_State* L, lua_Reader reader, void* dt,
	const char* chunkname, const char* mode) {
	static auto ptr = ((decltype(lua_load)*)get_proaddress("lua_load"));
	return ptr(L, reader, dt, chunkname, mode);
}
int lua_dump(lua_State* L, lua_Writer writer, void* data, int strip) {
	static auto ptr = ((decltype(lua_dump)*)get_proaddress("lua_dump"));
	return ptr(L, writer, data, strip);
}
int  lua_yieldk(lua_State* L, int nresults, lua_KContext ctx,
	lua_KFunction k) {
	static auto ptr = ((decltype(lua_yieldk)*)get_proaddress("lua_yieldk"));
	return ptr(L, nresults, ctx, k);
}
int  lua_resume(lua_State* L, lua_State* from, int narg) {
	static auto ptr = ((decltype(lua_resume)*)get_proaddress("lua_resume"));
	return ptr(L, from, narg);
}
int  lua_status(lua_State* L) {
	static auto ptr = ((decltype(lua_status)*)get_proaddress("lua_status"));
	return ptr(L);
}
int lua_isyieldable(lua_State* L) {
	static auto ptr = ((decltype(lua_isyieldable)*)get_proaddress("lua_isyieldable"));
	return ptr(L);
}
int lua_gc(lua_State* L, int what, int data) {
	static auto ptr = ((decltype(lua_gc)*)get_proaddress("lua_gc"));
	return ptr(L, what, data);
}
int   lua_error(lua_State* L) {
	static auto ptr = ((decltype(lua_error)*)get_proaddress("lua_error"));
	return ptr(L);
}
int   lua_next(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_next)*)get_proaddress("lua_next"));
	return ptr(L, idx);
}
void  lua_concat(lua_State* L, int n) {
	static auto ptr = ((decltype(lua_concat)*)get_proaddress("lua_concat"));
	ptr(L, n);
}
void  lua_len(lua_State* L, int idx) {
	static auto ptr = ((decltype(lua_len)*)get_proaddress("lua_len"));
	ptr(L, idx);
}
size_t   lua_stringtonumber(lua_State* L, const char* s) {
	static auto ptr = ((decltype(lua_stringtonumber)*)get_proaddress("lua_stringtonumber"));
	return ptr(L, s);
}
lua_Alloc lua_getallocf(lua_State* L, void** ud) {
	static auto ptr = ((decltype(lua_getallocf)*)get_proaddress("lua_getallocf"));
	return ptr(L, ud);
}
void      lua_setallocf(lua_State* L, lua_Alloc f, void* ud) {
	static auto ptr = ((decltype(lua_setallocf)*)get_proaddress("lua_setallocf"));
	ptr(L, f, ud);
}
int lua_getstack(lua_State* L, int level, lua_Debug* ar) {
	static auto ptr = ((decltype(lua_getstack)*)get_proaddress("lua_getstack"));
	return ptr(L, level, ar);
}
int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar) {
	static auto ptr = ((decltype(lua_getinfo)*)get_proaddress("lua_getinfo"));
	return ptr(L, what, ar);
}
const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n) {
	static auto ptr = ((decltype(lua_getlocal)*)get_proaddress("lua_getlocal"));
	return ptr(L, ar, n);
}
const char* lua_setlocal(lua_State* L, const lua_Debug* ar, int n) {
	static auto ptr = ((decltype(lua_setlocal)*)get_proaddress("lua_setlocal"));
	return ptr(L, ar, n);
}
const char* lua_getupvalue(lua_State* L, int funcindex, int n) {
	static auto ptr = ((decltype(lua_getupvalue)*)get_proaddress("lua_getupvalue"));
	return ptr(L, funcindex, n);
}
const char* lua_setupvalue(lua_State* L, int funcindex, int n) {
	static auto ptr = ((decltype(lua_setupvalue)*)get_proaddress("lua_setupvalue"));
	return ptr(L, funcindex, n);
}
void* lua_upvalueid(lua_State* L, int fidx, int n) {
	static auto ptr = ((decltype(lua_upvalueid)*)get_proaddress("lua_upvalueid"));
	return ptr(L, fidx, n);
}
void  lua_upvaluejoin(lua_State* L, int fidx1, int n1,
	int fidx2, int n2) {
	static auto ptr = ((decltype(lua_upvaluejoin)*)get_proaddress("lua_upvaluejoin"));
	ptr(L, fidx1, n1, fidx2, n2);
}
void lua_sethook(lua_State* L, lua_Hook func, int mask, int count) {
	static auto ptr = ((decltype(lua_sethook)*)get_proaddress("lua_sethook"));
	ptr(L, func, mask, count);
}
lua_Hook lua_gethook(lua_State* L) {
	static auto ptr = ((decltype(lua_gethook)*)get_proaddress("lua_gethook"));
	return ptr(L);
}
int lua_gethookmask(lua_State* L) {
	static auto ptr = ((decltype(lua_gethookmask)*)get_proaddress("lua_gethookmask"));
	return ptr(L);
}
int lua_gethookcount(lua_State* L) {
	static auto ptr = ((decltype(lua_gethookcount)*)get_proaddress("lua_gethookcount"));
	return ptr(L);
}
void luaL_checkversion_(lua_State* L, lua_Number ver, size_t sz) {
	static auto ptr = ((decltype(luaL_checkversion_)*)get_proaddress("luaL_checkversion_"));
	ptr(L, ver, sz);
}
int luaL_getmetafield(lua_State* L, int obj, const char* e) {
	static auto ptr = ((decltype(luaL_getmetafield)*)get_proaddress("luaL_getmetafield"));
	return ptr(L, obj, e);
}
int luaL_callmeta(lua_State* L, int obj, const char* e) {
	static auto ptr = ((decltype(luaL_callmeta)*)get_proaddress("luaL_callmeta"));
	return ptr(L, obj, e);
}
const char* luaL_tolstring(lua_State* L, int idx, size_t* len) {
	static auto ptr = ((decltype(luaL_tolstring)*)get_proaddress("luaL_tolstring"));
	return ptr(L, idx, len);
}
int luaL_argerror(lua_State* L, int arg, const char* extramsg) {
	static auto ptr = ((decltype(luaL_argerror)*)get_proaddress("luaL_argerror"));
	return ptr(L, arg, extramsg);
}
const char* luaL_checklstring(lua_State* L, int arg,
	size_t* l) {
	static auto ptr = ((decltype(luaL_checklstring)*)get_proaddress("luaL_checklstring"));
	return ptr(L, arg, l);
}
const char* luaL_optlstring(lua_State* L, int arg,
	const char* def, size_t* l) {
	static auto ptr = ((decltype(luaL_optlstring)*)get_proaddress("luaL_optlstring"));
	return ptr(L, arg, def, l);
}
lua_Number luaL_checknumber(lua_State* L, int arg) {
	static auto ptr = ((decltype(luaL_checknumber)*)get_proaddress("luaL_checknumber"));
	return ptr(L, arg);
}
lua_Number luaL_optnumber(lua_State* L, int arg, lua_Number def) {
	static auto ptr = ((decltype(luaL_optnumber)*)get_proaddress("luaL_optnumber"));
	return ptr(L, arg, def);
}
lua_Integer luaL_checkinteger(lua_State* L, int arg) {
	static auto ptr = ((decltype(luaL_checkinteger)*)get_proaddress("luaL_checkinteger"));
	return ptr(L, arg);
}
lua_Integer luaL_optinteger(lua_State* L, int arg,
	lua_Integer def) {
	static auto ptr = ((decltype(luaL_optinteger)*)get_proaddress("luaL_optinteger"));
	return ptr(L, arg, def);
}
void luaL_checkstack(lua_State* L, int sz, const char* msg) {
	static auto ptr = ((decltype(luaL_checkstack)*)get_proaddress("luaL_checkstack"));
	ptr(L, sz, msg);
}
void luaL_checktype(lua_State* L, int arg, int t) {
	static auto ptr = ((decltype(luaL_checktype)*)get_proaddress("luaL_checktype"));
	ptr(L, arg, t);
}
void luaL_checkany(lua_State* L, int arg) {
	static auto ptr = ((decltype(luaL_checkany)*)get_proaddress("luaL_checkany"));
	ptr(L, arg);
}
int   luaL_newmetatable(lua_State* L, const char* tname) {
	static auto ptr = ((decltype(luaL_newmetatable)*)get_proaddress("luaL_newmetatable"));
	return ptr(L, tname);
}
void  luaL_setmetatable(lua_State* L, const char* tname) {
	static auto ptr = ((decltype(luaL_setmetatable)*)get_proaddress("luaL_setmetatable"));
	ptr(L, tname);
}
void* luaL_testudata(lua_State* L, int ud, const char* tname) {
	static auto ptr = ((decltype(luaL_testudata)*)get_proaddress("luaL_testudata"));
	return ptr(L, ud, tname);
}
void* luaL_checkudata(lua_State* L, int ud, const char* tname) {
	static auto ptr = ((decltype(luaL_checkudata)*)get_proaddress("luaL_checkudata"));
	return ptr(L, ud, tname);
}
void luaL_where(lua_State* L, int lvl) {
	static auto ptr = ((decltype(luaL_where)*)get_proaddress("luaL_where"));
	ptr(L, lvl);
}
int luaL_error(lua_State* L, const char* fmt, ...) {
	static auto ptr = ((decltype(luaL_error)*)get_proaddress("luaL_error"));
	return ptr(L, fmt);
}
int luaL_checkoption(lua_State* L, int arg, const char* def,
	const char* const lst[]) {
	static auto ptr = ((decltype(luaL_checkoption)*)get_proaddress("luaL_checkoption"));
	return ptr(L, arg, def, lst);
}
int luaL_fileresult(lua_State* L, int stat, const char* fname) {
	static auto ptr = ((decltype(luaL_fileresult)*)get_proaddress("luaL_fileresult"));
	return ptr(L, stat, fname);
}
int luaL_execresult(lua_State* L, int stat) {
	static auto ptr = ((decltype(luaL_execresult)*)get_proaddress("luaL_execresult"));
	return ptr(L, stat);
}
int luaL_ref(lua_State* L, int t) {
	static auto ptr = ((decltype(luaL_ref)*)get_proaddress("luaL_ref"));
	return ptr(L, t);
}
void luaL_unref(lua_State* L, int t, int ref) {
	static auto ptr = ((decltype(luaL_unref)*)get_proaddress("luaL_unref"));
	ptr(L, t, ref);
}
int luaL_loadfilex(lua_State* L, const char* filename,
	const char* mode) {
	static auto ptr = ((decltype(luaL_loadfilex)*)get_proaddress("luaL_loadfilex"));
	return ptr(L, filename, mode);
}
int luaL_loadbufferx(lua_State* L, const char* buff, size_t sz,
	const char* name, const char* mode) {
	static auto ptr = ((decltype(luaL_loadbufferx)*)get_proaddress("luaL_loadbufferx"));
	return ptr(L, buff, sz, name, mode);
}
int luaL_loadstring(lua_State* L, const char* s) {
	static auto ptr = ((decltype(luaL_loadstring)*)get_proaddress("luaL_loadstring"));
	return ptr(L, s);
}
lua_State* luaL_newstate(void) {
	static auto ptr = ((decltype(luaL_newstate)*)get_proaddress("luaL_newstate"));
	return ptr();
}
lua_Integer luaL_len(lua_State* L, int idx) {
	static auto ptr = ((decltype(luaL_len)*)get_proaddress("luaL_len"));
	return ptr(L, idx);
}
const char* luaL_gsub(lua_State* L, const char* s, const char* p,
	const char* r) {
	static auto ptr = ((decltype(luaL_gsub)*)get_proaddress("luaL_gsub"));
	return ptr(L, s, p, r);
}
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup) {
	static auto ptr = ((decltype(luaL_setfuncs)*)get_proaddress("luaL_setfuncs"));
	ptr(L, l, nup);
}
int luaL_getsubtable(lua_State* L, int idx, const char* fname) {
	static auto ptr = ((decltype(luaL_getsubtable)*)get_proaddress("luaL_getsubtable"));
	return ptr(L, idx, fname);
}
void luaL_traceback(lua_State* L, lua_State* L1,
	const char* msg, int level) {
	static auto ptr = ((decltype(luaL_traceback)*)get_proaddress("luaL_traceback"));
	ptr(L, L1, msg, level);
}
void luaL_requiref(lua_State* L, const char* modname,
	lua_CFunction openf, int glb) {
	static auto ptr = ((decltype(luaL_requiref)*)get_proaddress("luaL_requiref"));
	ptr(L, modname, openf, glb);
}
void luaL_buffinit(lua_State* L, luaL_Buffer* B) {
	static auto ptr = ((decltype(luaL_buffinit)*)get_proaddress("luaL_buffinit"));
	ptr(L, B);
}
char* luaL_prepbuffsize(luaL_Buffer* B, size_t sz) {
	static auto ptr = ((decltype(luaL_prepbuffsize)*)get_proaddress("luaL_prepbuffsize"));
	return ptr(B, sz);
}
void luaL_addlstring(luaL_Buffer* B, const char* s, size_t l) {
	static auto ptr = ((decltype(luaL_addlstring)*)get_proaddress("luaL_addlstring"));
	ptr(B, s, l);
}
void luaL_addstring(luaL_Buffer* B, const char* s) {
	static auto ptr = ((decltype(luaL_addstring)*)get_proaddress("luaL_addstring"));
	ptr(B, s);
}
void luaL_addvalue(luaL_Buffer* B) {
	static auto ptr = ((decltype(luaL_addvalue)*)get_proaddress("luaL_addvalue"));
	ptr(B);
}
void luaL_pushresult(luaL_Buffer* B) {
	static auto ptr = ((decltype(luaL_pushresult)*)get_proaddress("luaL_pushresult"));
	ptr(B);
}
void luaL_pushresultsize(luaL_Buffer* B, size_t sz) {
	static auto ptr = ((decltype(luaL_pushresultsize)*)get_proaddress("luaL_pushresultsize"));
	ptr(B, sz);
}
char* luaL_buffinitsize(lua_State* L, luaL_Buffer* B, size_t sz) {
	static auto ptr = ((decltype(luaL_buffinitsize)*)get_proaddress("luaL_buffinitsize"));
	return ptr(L, B, sz);
}
void luaL_pushmodule(lua_State* L, const char* modname,
	int sizehint) {
	static auto ptr = ((decltype(luaL_pushmodule)*)get_proaddress("luaL_pushmodule"));
	ptr(L, modname, sizehint);
}
void luaL_openlib(lua_State* L, const char* libname,
	const luaL_Reg* l, int nup) {
	static auto ptr = ((decltype(luaL_openlib)*)get_proaddress("luaL_openlib"));
	ptr(L, libname, l, nup);
}
int luaopen_base(lua_State* L) {
	static auto ptr = ((decltype(luaopen_base)*)get_proaddress("luaopen_base"));
	return ptr(L);
}
int luaopen_coroutine(lua_State* L) {
	static auto ptr = ((decltype(luaopen_coroutine)*)get_proaddress("luaopen_coroutine"));
	return ptr(L);
}
int luaopen_table(lua_State* L) {
	static auto ptr = ((decltype(luaopen_table)*)get_proaddress("luaopen_table"));
	return ptr(L);
}
int luaopen_io(lua_State* L) {
	static auto ptr = ((decltype(luaopen_io)*)get_proaddress("luaopen_io"));
	return ptr(L);
}
int luaopen_os(lua_State* L) {
	static auto ptr = ((decltype(luaopen_os)*)get_proaddress("luaopen_os"));
	return ptr(L);
}
int luaopen_string(lua_State* L) {
	static auto ptr = ((decltype(luaopen_string)*)get_proaddress("luaopen_string"));
	return ptr(L);
}
int luaopen_utf8(lua_State* L) {
	static auto ptr = ((decltype(luaopen_utf8)*)get_proaddress("luaopen_utf8"));
	return ptr(L);
}
int luaopen_bit32(lua_State* L) {
	static auto ptr = ((decltype(luaopen_bit32)*)get_proaddress("luaopen_bit32"));
	return ptr(L);
}
int luaopen_math(lua_State* L) {
	static auto ptr = ((decltype(luaopen_math)*)get_proaddress("luaopen_math"));
	return ptr(L);
}
int luaopen_debug(lua_State* L) {
	static auto ptr = ((decltype(luaopen_debug)*)get_proaddress("luaopen_debug"));
	return ptr(L);
}
int luaopen_package(lua_State* L) {
	static auto ptr = ((decltype(luaopen_package)*)get_proaddress("luaopen_package"));
	return ptr(L);
}
void luaL_openlibs(lua_State* L) {
	static auto ptr = ((decltype(luaL_openlibs)*)get_proaddress("luaL_openlibs"));
	ptr(L);
}

