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

static const size_t maxStackDeep = 120;
size_t captureBacktrace(intptr_t* buffer, size_t maxStackDeep);
 
void dumpBacktraceIndex(char *out, intptr_t* buffer, size_t count,size_t mc);

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
    FILE* fp = fopen("/sdcard/stack_libtolua.txt","a");
    if(fp!=0){
      fprintf(fp,"%s","STACK FRAMES FOR LIBTOLUA\n");
      if(out!=0)
      {
          fprintf(fp,"%s\n",out);
      }
        for (size_t idx = 0; idx < count; ++idx) {
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
                fprintf(fp,"STACK: #%u:%p s:%s f:%s\n",real_idx,(void*)addr,symbol,dlfile);
            }else{
                fprintf(fp,"STACK: #%u:%p\n",real_idx,(void*)addr);
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
  
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "DOBBY lua_loadx(%s)", chunkname != 0 ? chunkname : "");

  intptr_t stackBuf[maxStackDeep] = {0};
  dumpBacktraceIndex(buffer, stackBuf, captureBacktrace(stackBuf, maxStackDeep), 0);


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
      fprintf(fp,"%s",splitter);
      fprintf(fp,"%s\n\n\n\n",name!=0?name:"");
      fwrite(buf,sizeof(char),size,fp);
      fclose(fp);
    }
}

#define JNI_VERSION_1_4 0x00010004

extern int JNI_OnLoad(void* jvm,void* reserved){
  return JNI_VERSION_1_4;
}
// typedef int (*UNWIND_DumpStack_Ptr)(const char* path);
// typedef int (*JNI_OnLoad_Ptr)(void*, void*);

// UNWIND_DumpStack_Ptr DumpStack = 0;
// JNI_OnLoad_Ptr JNI_OnLoad_Function=0;
//NOTICE: this is entry point:
LUALIB_API int luaL_loadbuffer(lua_State *L, const char *buf, size_t size,
			       const char *name)
{

  return luaL_loadbufferx(L, buf, size, name, NULL);
}

LUALIB_API int luaL_loadstring(lua_State *L, const char *s)
{

    // char buffer[1024];
    // memset(buffer,0,sizeof(buffer));
    // snprintf(buffer,sizeof(buffer),"DOBBY luaL_loadstring(%s)",s==0?"":s);
    
    // intptr_t stackBuf[maxStackDeep]={0};
    // dumpBacktraceIndex(buffer, stackBuf, captureBacktrace(stackBuf, maxStackDeep),0);


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


