/*
** Load and dump code.
** Copyright (C) 2005-2022 Mike Pall. See Copyright Notice in luajit.h
*/

#include <errno.h>
#include <stdio.h>

#define lj_load_c
#define LUA_CORE

#include "lua.h"
#include "lauxlib.h"

#include "lj_obj.h"
#include "lj_gc.h"
#include "lj_err.h"
#include "lj_buf.h"
#include "lj_func.h"
#include "lj_frame.h"
#include "lj_vm.h"
#include "lj_lex.h"
#include "lj_bcdump.h"
#include "lj_parse.h"

#include <android/log.h> 
#include <dlfcn.h>
#include <unwind.h>

#include "ExportFunctions.h"


// void PrintStackTrace()
// {
//   unw_context_t unw_ctx;
//   unw_cursor_t unw_cur;
//   unw_proc_info_t unw_proc;
//   unw_getcontext(&unw_ctx);
//   unw_init_local(&unw_cur, &unw_ctx);

//   char func_name_cache[4096];
//   func_name_cache[sizeof(func_name_cache) - 1] = 0;
//   unw_word_t unw_offset;
//   int frame_id = 0;
//   int skip_frames = 1 + static_cast<int>(options->skip_start_frames);
//   int frames_count = LOG_STACKTRACE_MAX_STACKS;

//   if (options->skip_end_frames > 0)
//   {
//     frames_count = 1;
//     while (unw_step(&unw_cur) > 0)
//     {
//       ++frames_count;
//     }

//     // restore cursor
//     unw_init_local(&unw_cur, &unw_ctx);

//     if (frames_count <= skip_frames + static_cast<int>(options->skip_end_frames))
//     {
//       frames_count = 0;
//     }
//     else
//     {
//       frames_count -= static_cast<int>(options->skip_end_frames);
//     }
//   }

//   size_t ret = 0;
//   do
//   {
//     if (frames_count <= 0)
//     {
//       break;
//     }
//     --frames_count;

//     if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames))
//     {
//       break;
//     }

//     if (skip_frames <= 0)
//     {
//       unw_get_proc_info(&unw_cur, &unw_proc);
//       if (0 == unw_proc.start_ip)
//       {
//         break;
//       }
//       unw_get_proc_name(&unw_cur, func_name_cache, sizeof(func_name_cache) - 1, &unw_offset);

//       const char *func_name = func_name_cache;
// #if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
//       int cxx_abi_status;
//       char *realfunc_name = abi::__cxa_demangle(func_name_cache, 0, 0, &cxx_abi_status);
//       if (NULL != realfunc_name)
//       {
//         func_name = realfunc_name;
//       }
// #endif

//       int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s+0x%llx) [0x%llx]\r\n", frame_id, func_name,
//                                       static_cast<unsigned long long>(unw_offset),
//                                       static_cast<unsigned long long>(unw_proc.start_ip));

//       if (res <= 0)
//       {
//         break;
//       }

//       ret += static_cast<size_t>(res);
//       buf += res;
//       bufsz -= static_cast<size_t>(res);

// #if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
//       if (NULL != realfunc_name)
//       {
//         free(realfunc_name);
//         realfunc_name = NULL;
//       }
// #endif
//     }

//     if (unw_step(&unw_cur) <= 0)
//     {
//       break;
//     }

//     if (skip_frames > 0)
//     {
//       --skip_frames;
//     }
//     else
//     {
//       ++frame_id;
//     }
//   } while (true);

//   return ret;
// }

const size_t maxStackDeep = 12;

typedef struct _BacktraceState
{
    intptr_t* current;
    intptr_t* end;
}BacktraceState;

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = (BacktraceState*)(arg);
    intptr_t ip = (intptr_t)_Unwind_GetIP(context);
    if (ip) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            state->current[0] = ip;
            state->current++;
        }
    }
    return _URC_NO_REASON;
 
 
}
 
size_t captureBacktrace(intptr_t* buffer, size_t maxStackDeep)
{
    BacktraceState state = {buffer, buffer + maxStackDeep};
    _Unwind_Backtrace(unwindCallback, &state);
    return state.current - buffer;
}

void dumpBacktraceIndex(char *out, intptr_t* buffer, size_t count,size_t mc)
{
    unsigned int real_idx = 0;
    size_t idx = 0;
    FILE* fp = fopen("/sdcard/result.txt","a");
    if(fp!=0){
      fprintf(fp,"DOBBY STACK TRACING\n");



      //__android_log_print(ANDROID_LOG_VERBOSE,"DOBBY", "DOBBY STACK TRACING");
      for (idx = 0; idx < count; ++idx) {
          intptr_t addr = buffer[idx];
          const char* symbol = "";
          const char* dlfile = "";
  
          Dl_info info;
          memset(&info,0,sizeof(info));
          if (dladdr((void*)addr, &info)) {
              if(info.dli_sname){
                symbol = info.dli_sname;
              }
              if(info.dli_fname){
                  dlfile = info.dli_fname;
              }            
              fprintf(fp,"DOBBY STACK: #%u:%p s:%s f:%s\n",real_idx,(void*)addr,symbol,dlfile);
              //__android_log_print(ANDROID_LOG_VERBOSE,"DOBBY", "DOBBY STACK: #%u:%p s:%s f:%s",real_idx,(void*)addr,symbol,dlfile);
          }else{
              fprintf(fp,"DOBBY STACK: #%u:%p",real_idx,(void*)addr);
              //__android_log_print(ANDROID_LOG_VERBOSE,"DOBBY", "DOBBY STACK: #%u:%p",real_idx,(void*)addr);
          }
          ++real_idx;        
      }
      fclose(fp);

    }
}


/* -- Load Lua source code and bytecode ----------------------------------- */

static TValue *cpparser(lua_State *L, lua_CFunction dummy, void *ud)
{
  LexState *ls = (LexState *)ud;
  GCproto *pt;
  GCfunc *fn;
  int bc;
  UNUSED(dummy);
  cframe_errfunc(L->cframe) = -1;  /* Inherit error function. */
  bc = lj_lex_setup(L, ls);
  if (ls->mode && !strchr(ls->mode, bc ? 'b' : 't')) {
    setstrV(L, L->top++, lj_err_str(L, LJ_ERR_XMODE));
    lj_err_throw(L, LUA_ERRSYNTAX);
  }
  pt = bc ? lj_bcread(ls) : lj_parse(ls);
  fn = lj_func_newL_empty(L, pt, tabref(L->env));
  /* Don't combine above/below into one statement. */
  setfuncV(L, L->top++, fn);
  return NULL;
}

LUA_API int lua_loadx(lua_State *L, lua_Reader reader, void *data,
		      const char *chunkname, const char *mode)
{
  LexState ls;
  int status;
  
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_loadx SOURCE CODE2:%s",chunkname!=0?chunkname:"");

  ls.rfunc = reader;
  ls.rdata = data;
  ls.chunkarg = chunkname ? chunkname : "?";
  ls.mode = mode;
  lj_buf_init(L, &ls.sb);
  status = lj_vm_cpcall(L, NULL, &ls, cpparser);
  lj_lex_cleanup(L, &ls);
  lj_gc_check(L);
  return status;
}

LUA_API int lua_load(lua_State *L, lua_Reader reader, void *data,
		     const char *chunkname)
{
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_load SOURCE CODE2:%s",chunkname!=0?chunkname:"");
  return lua_loadx(L, reader, data, chunkname, NULL);
}

typedef struct FileReaderCtx {
  FILE *fp;
  char buf[LUAL_BUFFERSIZE];
} FileReaderCtx;

static const char *reader_file(lua_State *L, void *ud, size_t *size)
{
  FileReaderCtx *ctx = (FileReaderCtx *)ud;
  UNUSED(L);
  if (feof(ctx->fp)) return NULL;
  *size = fread(ctx->buf, 1, sizeof(ctx->buf), ctx->fp);
  return *size > 0 ? ctx->buf : NULL;
}

LUALIB_API int luaL_loadfilex(lua_State *L, const char *filename,
			      const char *mode)
{
  FileReaderCtx ctx;
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_filex SOURCE CODE2:%s",filename!=0?filename:"");
  int status;
  const char *chunkname;
  if (filename) {
    ctx.fp = fopen(filename, "rb");
    if (ctx.fp == NULL) {
      lua_pushfstring(L, "cannot open %s: %s", filename, strerror(errno));
      return LUA_ERRFILE;
    }
    chunkname = lua_pushfstring(L, "@%s", filename);
  } else {
    ctx.fp = stdin;
    chunkname = "=stdin";
  }
  status = lua_loadx(L, reader_file, &ctx, chunkname, mode);
  if (ferror(ctx.fp)) {
    L->top -= filename ? 2 : 1;
    lua_pushfstring(L, "cannot read %s: %s", chunkname+1, strerror(errno));
    if (filename)
      fclose(ctx.fp);
    return LUA_ERRFILE;
  }
  if (filename) {
    L->top--;
    copyTV(L, L->top-1, L->top);
    fclose(ctx.fp);
  }
  return status;
}

LUALIB_API int luaL_loadfile(lua_State *L, const char *filename)
{
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_file SOURCE CODE2:%s",filename!=0?filename:"");
  return luaL_loadfilex(L, filename, NULL);
}

typedef struct StringReaderCtx {
  const char *str;
  size_t size;
} StringReaderCtx;

static const char *reader_string(lua_State *L, void *ud, size_t *size)
{
  StringReaderCtx *ctx = (StringReaderCtx *)ud;
  UNUSED(L);
  if (ctx->size == 0) return NULL;
  *size = ctx->size;
  ctx->size = 0;
  return ctx->str;
}

LUALIB_API int luaL_loadbufferx(lua_State *L, const char *buf, size_t size,
				const char *name, const char *mode)
{
  StringReaderCtx ctx;
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_bufferx SOURCE CODE2:%s",name!=0?name:"");
  ctx.str = buf;
  ctx.size = size;
  return lua_loadx(L, reader_string, &ctx, name, mode);
}

const char* ResultFilePath = "/sdcard/LuaResult.txt";

char splitter[256]={"\n====00000000====FFFFFFFF====00000000====FFFFFFFF\n"};

void lua_savebuffer(const char* buf, size_t size, const char* name){
    FILE* fp = fopen(ResultFilePath,"a");
    if(fp!=0){
      fprintf(fp,splitter);
      fprintf(fp,"%s\n\n\n\n",name!=0?name:"");
      fwrite(buf,sizeof(char),size,fp);
      fclose(fp);
    }
}

typedef int (*UNWIND_DumpStack_Ptr)(const char* path);
typedef int (*JNI_OnLoad_Ptr)(void*, void*);

UNWIND_DumpStack_Ptr DumpStack = 0;
JNI_OnLoad_Ptr JNI_OnLoad_Function=0;
//NOTICE: this is entry point:
LUALIB_API int luaL_loadbuffer(lua_State *L, const char *buf, size_t size,
			       const char *name)
{
  //HERE:
  //lua_savebuffer(buf,size,name);

#if 1
  UNWIND_DumpStack("/sdcard/DOBBY_STACK.txt");
#else
  if(DumpStack==0 && name!=0&&name[0]=='@'){
    __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_loadbuffer NEW:%s",name!=0?name:"");
#if 0 
    void* handle = dlopen("libunwindstack.so",RTLD_NOW);
    if(handle!=0){
      DumpStack = (UNWIND_DumpStack_Ptr)dlsym(handle,"UNWIND_DumpStack");
      if(DumpStack!=0){
      __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY found UNWIND_DumpStack");
      }
      dlclose(handle);
    }
#else
    void* handle = dlopen("libdumb.so",RTLD_NOW);
    if(handle!=0){
      JNI_OnLoad_Function = (JNI_OnLoad_Ptr)dlsym(handle,"JNI_OnLoad");
      if(DumpStack!=0){
        __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY found libdumb.so:JNI_OnLoad");
      }
      dlclose(handle);
    }

#endif
  }
#if 0
  if(DumpStack!=0){
    DumpStack("/sdcard/DOBBY_STACK.txt");
  }
#else
  if(JNI_OnLoad_Function!=0){
    __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY call libdumb.so JNI_OnLoad");
    JNI_OnLoad_Function(0,0);
    __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY call libdumb.so JNI_OnLoad DONE");
  }
#endif
#endif
  // intptr_t stackBuf[maxStackDeep];
  // memset(stackBuf,0,sizeof(stackBuf));
  // dumpBacktraceIndex(0, stackBuf, captureBacktrace(stackBuf, maxStackDeep),0);  

  return luaL_loadbufferx(L, buf, size, name, NULL);
}

LUALIB_API int luaL_loadstring(lua_State *L, const char *s)
{
  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_loadstring SOURCE CODE2:%s",s!=0?s:"");
  return luaL_loadbuffer(L, s, strlen(s), s);
}

/* -- Dump bytecode ------------------------------------------------------- */

LUA_API int lua_dump(lua_State *L, lua_Writer writer, void *data)
{
  cTValue *o = L->top-1;

  // __android_log_print(ANDROID_LOG_VERBOSE, "DOBBY", "DOBBY lua_dump SOURCE CODE2");

  lj_checkapi(L->top > L->base, "top slot empty");
  if (tvisfunc(o) && isluafunc(funcV(o)))
    return lj_bcwrite(L, funcproto(funcV(o)), writer, data, 0);
  else
    return 1;
}

//calling
//buffer->bufferx->loadx


