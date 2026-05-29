#ifndef MINI_SCHEME_MSCM_H
#define MINI_SCHEME_MSCM_H

/* mscm.h - Mini Scheme 可嵌入引擎的统一公共 API
 *
 * 使用方法:
 *   #include "mscm.h"
 *
 *   mscm_engine *engine = mscm_engine_create();
 *   mscm_engine_load_ext(engine, "libcore.so");
 *   mscm_value result = mscm_engine_eval_string(engine, "(+ 1 2)");
 *   mscm_engine_destroy(engine);
 *
 * 这个头文件将解释器核心封装为可复用组件，不依赖任何 CLI 逻辑。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rt.h"
#include "scope.h"
#include "value.h"
#include "slice.h"
#include "dump.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 引擎 (Engine) ========== */

/* 引擎是对运行时、解析器和扩展加载的高层封装 */
typedef struct st_mscm_engine mscm_engine;

/* 创建一个新的引擎实例
 *
 * 返回 NULL 表示内存分配失败。
 */
mscm_engine *mscm_engine_create(void);

/* 销毁引擎实例，释放所有关联资源 */
void mscm_engine_destroy(mscm_engine *engine);

/* 获取引擎内部的运行时指针
 *
 * 可用于直接调用底层 runtime API（如 mscm_runtime_push 等）。
 */
mscm_runtime *mscm_engine_get_runtime(mscm_engine *engine);

/* ========== 代码求值 ========== */

/* 解析并求值一段源代码字符串
 *
 * file_name 用于错误报告中的文件名（可以是 "<string>" 等）。
 * 返回最后一个表达式的求值结果，出错时返回 NULL。
 */
mscm_value mscm_engine_eval_string(mscm_engine *engine,
                                   const char *file_name,
                                   const char *source);

/* 从文件中读取源代码并求值
 *
 * 返回最后一个表达式的求值结果，出错时返回 NULL。
 */
mscm_value mscm_engine_eval_file(mscm_engine *engine,
                                 const char *file_path);

/* ========== 扩展加载 ========== */

/* 加载一个动态链接库扩展（.so / .dll）
 *
 * 扩展必须导出 void mscm_load_ext(mscm_runtime *rt) 函数。
 * 成功返回 true，失败返回 false。
 */
bool mscm_engine_load_ext(mscm_engine *engine,
                          const char *lib_path);

/* 直接调用一个扩展加载函数（无需动态链接）
 *
 * 适用于静态链接扩展或测试场景。
 */
typedef void (*mscm_ext_loader_fn)(mscm_runtime *rt);
void mscm_engine_load_ext_fn(mscm_engine *engine,
                             mscm_ext_loader_fn loader);

/* ========== 便捷绑定 ========== */

/* 向全局作用域注册一个 native 函数 */
void mscm_engine_define_native(mscm_engine *engine,
                               const char *name,
                               mscm_native_fnptr fnptr,
                               void *ctx,
                               mscm_user_dtor ctx_dtor,
                               mscm_user_marker ctx_marker);

/* 向全局作用域注册一个值 */
void mscm_engine_define(mscm_engine *engine,
                        const char *name,
                        mscm_value value);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MINI_SCHEME_MSCM_H */
