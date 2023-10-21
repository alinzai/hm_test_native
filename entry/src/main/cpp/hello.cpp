#include "napi/native_api.h"

#include <string>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <thread>
#define LOG_DOMAIN 0x99
#define LOG_TAG "hello"
#include <hilog/log.h>

#define NAPI_DESC(name, func) \
  napi_property_descriptor{ name, 0, func, 0, 0, 0, napi_default, 0 }

#define NAPI_DESC_Data(name, func, data) \
  napi_property_descriptor{ name, 0, func, 0, 0, 0, napi_default, data }

#define CHECK(expr) \
  { \
    if ((expr == napi_ok) == 0) { \
        OH_LOG_INFO(LOG_APP, "[Err] %{public}s:%{public}d: %{public}s\n", __FILE__, __LINE__, #expr);\
    } \
}

#define Logout(str) { \
    OH_LOG_INFO(LOG_APP, "[Logout]  %{public}s:%{public}d:%{public}s\n", __FILE__, __LINE__, str);\
  }
#define LogoutInt(nValue) { \
    OH_LOG_INFO(LOG_APP, "[LogoutInt]  %{public}ld\n", nValue);\
}

static napi_value Add(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;

}

struct CallbackData{
  CallbackData(): tsfn(nullptr) {}
  napi_threadsafe_function tsfn;
};
CallbackData* g_cb_data;
static void PostSvcRsp(napi_env env, napi_value js_cb, void* context, void* pData) {
  //Logout((char*)pData);

    if (env == nullptr || js_cb == nullptr) {
         OH_LOG_INFO(LOG_APP, "[PostSvcRsp]  js_cb is nullptr\n");
        return ;
    }
    {
        const napi_extended_error_info *error_info;
        napi_get_last_error_info((env), &error_info);
        const char *err_message = error_info->error_message;
        OH_LOG_INFO(LOG_APP, "[PostSvcRsp]  err_message=%{public}s\n", err_message);
    }
    napi_valuetype valuetype0;
    CHECK(napi_typeof(env, js_cb, &valuetype0));
    OH_LOG_INFO(LOG_APP, "[PostSvcRsp]  valuetype0=%{public}d\n", valuetype0);

    //if (valuetype0 == napi_function) {
        napi_value argv[1];
        CHECK(napi_create_string_utf8(env, (char *)pData, NAPI_AUTO_LENGTH, argv));
        napi_value undefined;
        CHECK(napi_get_undefined(env, &undefined));
        napi_value result;
        
        auto ret = napi_call_function(env, undefined, js_cb, 1, argv, &result);
        OH_LOG_INFO(LOG_APP, "[PostSvcRsp]  ret=%{public}d\n", ret);
        {
            const napi_extended_error_info *error_info;
            napi_get_last_error_info((env), &error_info);
            const char *err_message = error_info->error_message;
            OH_LOG_INFO(LOG_APP, "[PostSvcRsp]  err_message=%{public}s\n", err_message);
        }
        //CHECK(ret);
        //Logout(result);
        delete[] pData;
        
    //}
    
}


napi_value RegSvcRsp(napi_env env, napi_callback_info info) {
    napi_value work_name;
    CallbackData* cb_data;
    size_t argc = 1;
    napi_value js_cb;
    CHECK(napi_get_cb_info(env, info, &argc, &js_cb, NULL, (void **)(&cb_data)));

    napi_valuetype valuetype0;
    napi_typeof(env, js_cb, &valuetype0);
    OH_LOG_INFO(LOG_APP, "[RegSvcRsp]  js_cb valuetype=%{public}d argc=%{public}d cb_data->tsfn=%{public}p\n", valuetype0, argc, &(cb_data->tsfn));

    CHECK(napi_create_string_utf8(env, "CallAsyncWork", NAPI_AUTO_LENGTH, &work_name));
    CHECK(napi_create_threadsafe_function(env, js_cb, NULL, work_name,
                                          0, 1, NULL, NULL, NULL, PostSvcRsp, &(cb_data->tsfn)));

    {
            const napi_extended_error_info *error_info;
            napi_get_last_error_info((env), &error_info);
            const char *err_message = error_info->error_message;
            OH_LOG_INFO(LOG_APP, "[RegSvcRsp]  err_message=%{public}s\n", err_message);
    }
    std::thread t([] {
        int i = 0;
        CHECK(napi_acquire_threadsafe_function(g_cb_data->tsfn));
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // send resp in another thread,
            char *p = new char[200];
            sprintf(p, "retStr=%d", i++);
            OH_LOG_INFO(LOG_APP, "[thread_t]  [ cb_data->tsfn=%{public}p\n", &g_cb_data->tsfn);

            CHECK(napi_call_threadsafe_function(g_cb_data->tsfn, (void *)p, napi_tsfn_blocking));
            //
            OH_LOG_INFO(LOG_APP, "[thread_t]  ]\n");
        }
        napi_release_threadsafe_function(g_cb_data->tsfn, napi_tsfn_release);
    });
    t.detach();
    return nullptr;
}

//napi_value UnregSvcRsp(napi_env env, napi_callback_info info) {
//
//  CHECK(napi_release_threadsafe_function(g_cb_data->tsfn, napi_tsfn_release));
//  // 这里没启动work, 所以不需要删除work
//  // CHECK(napi_delete_async_work(env, g_cb_data->work));
//  g_cb_data->tsfn = nullptr;
//  delete g_cb_data;
//  return nullptr;
//}


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    g_cb_data = new CallbackData();  
    napi_property_descriptor desc[] = {
        { "add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "RegSvcRsp", nullptr, RegSvcRsp, nullptr, nullptr, nullptr, napi_default, g_cb_data },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version =1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
