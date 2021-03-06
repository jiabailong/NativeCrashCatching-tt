//
// Created by Jekton Luo on 2019/4/4.
//
#include "backtrace.h"
#include "backtrace_constants.h"
#include "backtrace_impl.h"

#include <dlfcn.h>
#include <ucontext.h>

#include <memory>
#include <limits>
#include <sstream>

#include "../dlopen.h"
#include "../log_util.h"
#include "../util.h"

static const char* kTag = "Backtrace";
const char* kLibBacktrace = "libbacktrace.so";

static BacktraceStub* CreateBacktrace(pid_t tid);
static uint64_t GetFaultPcRelative(ucontext_t* context);

void GetStackTrace(pid_t tid, void* ctx, GetTraceCallback* callback) {
  std::unique_ptr<BacktraceStub> backtrace{CreateBacktrace(tid)};
  if (!backtrace) {
    callback->OnFail();
    return;
  }
  const auto ignoreDepth = 0;
  if (!backtrace->Unwind(ignoreDepth)) {
    LOGE(kTag, "GetStackTrace, fail to unwind stack");
    callback->OnFail();
    return;
  }

  auto context = reinterpret_cast<ucontext_t*>(ctx);
  // uc_mcontext.pc is the next instruction to be executed
  auto pc = static_cast<uint64_t>(context->uc_mcontext.pc-4) ;
  size_t j = 0;
  std::ostringstream oss;
  for (size_t i = 0, size = backtrace->NumFrames(); i < size; ++i) {
    auto frame = backtrace->GetFrame(i);

    // skip frames due to notification of dumping thread
    const_cast<backtrace_frame_data_t*>(frame)->num = j;
    auto frame_str = backtrace->FormatFrameData(i);
    ++j;
    oss<<i<<frame_str<<"\n";

  }
  callback->OnFrame(1, oss.str());

  LOGE(kTag, "GetStackTrace, over");
}

static BacktraceStub* CreateBacktrace(pid_t tid) {
  auto deleter = [](void* handle) { ndk_dlclose(handle); };
  std::unique_ptr<void, decltype(deleter)> handle{ndk_dlopen(kLibBacktrace, RTLD_LAZY), deleter};
  if (!handle) {
    LOGERRNO(kTag, "CrateBacktrace, fail to dlopen %s", kLibBacktrace);
    return nullptr;
  }

  using BacktraceCreate = BacktraceStub* (*)(pid_t pid, pid_t tid, void* map);
  union { void* p; BacktraceCreate fn; } backtrace_create;
  backtrace_create.p = ndk_dlsym(handle.get(), "_ZN9Backtrace6CreateEiiP12BacktraceMap");
  if (!backtrace_create.p) {
    LOGE(kTag, "CrateBacktrace, fail to get symbol Backtrace::Create: %s", ndk_dlerror());
    return nullptr;
  }

  return backtrace_create.fn(BACKTRACE_CURRENT_PROCESS, tid, nullptr);
}

static uint64_t GetFaultPcRelative(ucontext_t* context) {
  void* pc = reinterpret_cast<void*>(context->uc_mcontext.pc);
  Dl_info dl_info;
  if (dladdr(pc, &dl_info)) {
    auto base = reinterpret_cast<uint64_t>(dl_info.dli_fbase);
    return reinterpret_cast<uint64_t>(pc) - base;
  } else {
    return 0;
  }
}

