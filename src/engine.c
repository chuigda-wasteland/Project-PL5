#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mscm.h"
#include "rt_impl.h"
#include "parse.h"
#include "syntax.h"

#ifdef _WIN32
#   include <windows.h>
#else
#   include <dlfcn.h>
#endif

/* ========== 引擎结构体 ========== */

struct st_mscm_engine {
    mscm_runtime *rt;

    /* 保存所有已解析的语法树，用于最终释放 */
    mscm_syntax_node syntax_head;
    mscm_syntax_node syntax_tail;
};

/* ========== 内部辅助函数 ========== */

static mscm_syntax_node find_last(mscm_syntax_node node) {
    while (node->next) {
        node = node->next;
    }
    return node;
}

static void engine_track_syntax(mscm_engine *engine,
                                mscm_syntax_node node) {
    if (engine->syntax_head) {
        engine->syntax_tail->next = node;
        engine->syntax_tail = find_last(node);
    }
    else {
        engine->syntax_head = node;
        engine->syntax_tail = find_last(node);
    }
}

static char *read_file_to_string(const char *file) {
    FILE *f = fopen(file, "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

/* ========== 公共 API 实现 ========== */

mscm_engine *mscm_engine_create(void) {
    mscm_engine *engine = malloc(sizeof(mscm_engine));
    if (!engine) {
        return NULL;
    }

    engine->rt = runtime_new();
    if (!engine->rt) {
        free(engine);
        return NULL;
    }

    engine->syntax_head = NULL;
    engine->syntax_tail = NULL;
    return engine;
}

void mscm_engine_destroy(mscm_engine *engine) {
    if (!engine) {
        return;
    }

    runtime_free(engine->rt);
    mscm_free_syntax_node(engine->syntax_head);
    free(engine);
}

mscm_runtime *mscm_engine_get_runtime(mscm_engine *engine) {
    return engine->rt;
}

mscm_value mscm_engine_eval_string(mscm_engine *engine,
                                   const char *file_name,
                                   const char *source) {
    mscm_syntax_node node = mscm_parse(file_name, source);
    if (!node) {
        return NULL;
    }

    engine_track_syntax(engine, node);
    return runtime_eval_entry(engine->rt, node);
}

mscm_value mscm_engine_eval_file(mscm_engine *engine,
                                 const char *file_path) {
    char *content = read_file_to_string(file_path);
    if (!content) {
        fprintf(stderr,
                "mscm_engine_eval_file: could not read file %s\n",
                file_path);
        return NULL;
    }

    mscm_syntax_node node = mscm_parse(file_path, content);
    free(content);

    if (!node) {
        fprintf(stderr,
                "mscm_engine_eval_file: could not parse file %s\n",
                file_path);
        return NULL;
    }

    engine_track_syntax(engine, node);
    return runtime_eval_entry(engine->rt, node);
}

bool mscm_engine_load_ext(mscm_engine *engine,
                          const char *lib_path) {
#ifdef _WIN32
    HANDLE h = LoadLibraryA(lib_path);
    if (!h) {
        fprintf(stderr,
                "mscm_engine_load_ext: could not load library %s\n",
                lib_path);
        return false;
    }

    mscm_ext_loader_fn loader =
        (mscm_ext_loader_fn)GetProcAddress(h, "mscm_load_ext");
    if (!loader) {
        fprintf(stderr,
                "mscm_engine_load_ext: "
                "could not locate mscm_load_ext in %s\n",
                lib_path);
        FreeLibrary(h);
        return false;
    }
#else
    void *h = dlopen(lib_path, RTLD_LAZY | RTLD_DEEPBIND);
    if (!h) {
        fprintf(stderr,
                "mscm_engine_load_ext: could not load library %s\n",
                lib_path);
        return false;
    }

    mscm_ext_loader_fn loader =
        (mscm_ext_loader_fn)dlsym(h, "mscm_load_ext");
    if (!loader) {
        fprintf(stderr,
                "mscm_engine_load_ext: "
                "could not locate mscm_load_ext in %s\n",
                lib_path);
        dlclose(h);
        return false;
    }
#endif

    loader(engine->rt);
    return true;
}

void mscm_engine_load_ext_fn(mscm_engine *engine,
                             mscm_ext_loader_fn loader) {
    loader(engine->rt);
}

void mscm_engine_define_native(mscm_engine *engine,
                               const char *name,
                               mscm_native_fnptr fnptr,
                               void *ctx,
                               mscm_user_dtor ctx_dtor,
                               mscm_user_marker ctx_marker) {
    mscm_value fn = mscm_make_native_function(name, fnptr,
                                              ctx, ctx_dtor,
                                              ctx_marker);
    mscm_gc_add(engine->rt, fn);
    mscm_runtime_push(engine->rt, name, fn);
}

void mscm_engine_define(mscm_engine *engine,
                        const char *name,
                        mscm_value value) {
    mscm_runtime_push(engine->rt, name, value);
}
