//
// Created by Jekton Luo on 2019/4/3.
//

#include "crash_catching.h"

#include <dlfcn.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <sstream>
#include <thread>
#include <fstream>
#include <iostream>
#include "backtrace/backtrace.h"
#include "dlopen.h"
#include "log_util.h"
#include "util.h"

static const char *kTag = "@ATI";

static std::map<int, struct sigaction> sOldHandlers;

static void SetUpStack();

static void SetUpSigHandler();

static void CallOldHandler(int signo, siginfo_t *info, void *context);

static void SignalHandler(int signo, siginfo_t *info, void *context);

static void DumpSignalInfo(siginfo_t *info);

static void DumpStacks(void *context);

static pid_t sTidToDump;    // guarded by sMutex
static void *sContext;

static void StackDumpingThread();

static JNIEnv *local_env;

void InitCrashCaching(JNIEnv *env) {
    LOGD(kTag, "InitCrashCaching");
    local_env = env;
    SetUpStack();
    SetUpSigHandler();
}

static void SetUpStack() {
    stack_t stack{};
    stack.ss_sp = new(std::nothrow) char[SIGSTKSZ];

    if (!stack.ss_sp) {
        LOGW(kTag, "fail to alloc stack for crash catching");
        return;
    }
    stack.ss_size = SIGSTKSZ;
    stack.ss_flags = 0;
    if (stack.ss_sp) {
        if (sigaltstack(&stack, nullptr) != 0) {
            LOGERRNO(kTag, "fail to setup signal stack");
        }
    }
}

static void SetUpSigHandler() {
    struct sigaction action{};
    action.sa_sigaction = SignalHandler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    int signals[] = {
            SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGPIPE
    };
    struct sigaction old_action;
    for (auto signo : signals) {
        if (sigaction(signo, &action, &old_action) == -1) {
            LOGERRNO(kTag, "fail to set signal handler for signo %d", signo);
        } else {
            if (old_action.sa_handler != SIG_DFL && old_action.sa_handler != SIG_IGN) {
                sOldHandlers[signo] = old_action;
            }
        }
    }
}


static void SignalHandler(int signo, siginfo_t *info, void *context) {
    DumpSignalInfo(info);
    DumpStacks(context);
    CallOldHandler(signo, info, context);
}

static void CallOldHandler(int signo, siginfo_t *info, void *context) {


    auto it = sOldHandlers.find(signo);
    if (it != sOldHandlers.end()) {
        if (it->second.sa_flags & SA_SIGINFO) {
            it->second.sa_sigaction(signo, info, context);
        } else {
            it->second.sa_handler(signo);
        }
    }
}
static const char *getTimeFileNmae() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    std::string str(buffer);
    std::ostringstream oss;
    oss << "/sdcard/CrmMatrix/cmdlog/";
    oss << str.c_str() << "nativeCrash.txt";
    std::string filename = oss.str();
    LOGE("@ati", "--name=%s", filename.c_str());
    const char *path = filename.c_str();
    return path;
}
static void writeStringToFIle(const std::string &str){
    const char *path = getTimeFileNmae();
    FILE *file = fopen(path, "a+");
    if (!file) {
        LOGD("@ATI", "打开失败");
    } else {
        LOGD("@ATI", "打开成功");

    }
    int len = strlen(str.c_str());
    LOGD("@ATI", "打开成功%d", len);

    fwrite(str.c_str(), len, sizeof(char), file);
    fclose(file);
}
static void DumpSignalInfo(siginfo_t *info) {
    std::ostringstream oss;

    switch (info->si_signo) {

        case SIGILL:
            LOGI(kTag, "signal SIGILL caught ");
            oss<<"signal SIGILL caught ";

            switch (info->si_code) {
                case ILL_ILLOPC:
                    LOGI(kTag, "illegal opcode");
                    oss<<"illegal opcode";
                    break;
                case ILL_ILLOPN:
                    LOGI(kTag, "illegal operand");
                    oss<<"illegal operand";
                    break;
                case ILL_ILLADR:

                    LOGI(kTag, "illegal addressing mode");
                    oss<<"illegal addressing mode";
                    break;
                case ILL_ILLTRP:
                    LOGI(kTag, "illegal trap");
                    oss<<"illegal trap";
                    break;
                case ILL_PRVOPC:
                    LOGI(kTag, "privileged opcode");
                    oss<<"privileged opcode";


                    break;
                case ILL_PRVREG:
                    LOGI(kTag, "privileged register");
                    oss<<"privileged register";

                    break;
                case ILL_COPROC:
                    LOGI(kTag, "coprocessor error");
                    oss<<"coprocessor error";

                    break;
                case ILL_BADSTK:
                    LOGI(kTag, "internal stack error");
                    oss<<"internal stack error";


                    break;
                default:
                    LOGI(kTag, "code = %d", info->si_code);
                    break;
            }
            break;
        case SIGFPE:
            LOGI(kTag, "signal SIGFPE caught");
            oss<<"signal SIGFPE caught ";

            switch (info->si_code) {
                case FPE_INTDIV:
                    LOGI(kTag, "integer divide by zero");
                    oss<<"integer divide by zero ";

                    break;
                case FPE_INTOVF:
                    LOGI(kTag, "integer overflow");
                    oss<<"integer overflow ";
                    break;
                case FPE_FLTDIV:
                    LOGI(kTag, "floating-point divide by zero");
                    oss<<"floating-point divide by zero ";
                    break;
                case FPE_FLTOVF:
                    LOGI(kTag, "floating-point overflow");
                    oss<<"floating-point overflow ";
                    break;
                case FPE_FLTUND:
                    LOGI(kTag, "floating-point underflow");
                    oss<<"floating-point underflow ";
                    break;
                case FPE_FLTRES:
                    LOGI(kTag, "floating-point inexact result");
                    oss<<"floating-point inexact result ";

                    break;
                case FPE_FLTINV:
                    LOGI(kTag, "invalid floating-point operation");
                    oss<<"invalid floating-point operation ";

                    break;
                case FPE_FLTSUB:
                    LOGI(kTag, "subscript out of range");
                    oss<<"subscript out of range ";

                    break;
                default:
                    LOGI(kTag, "code = %d", info->si_code);
                    break;
            }
            break;
        case SIGSEGV:
            LOGI(kTag, "signal SIGSEGV caught ");
            oss<<"signal SIGSEGV caught ";

            switch (info->si_code) {
                case SEGV_MAPERR:
                    LOGI(kTag, "address not mapped to object");
                    oss<<"address not mapped to object ";

                    break;
                case SEGV_ACCERR:
                    LOGI(kTag, "invalid permissions for mapped object");
                    oss<<"invalid permissions for mapped object ";

                    break;
                default:
                    LOGI(kTag, "code = %d", info->si_code);
                    break;
            }
            break;
        case SIGBUS:
            LOGI(kTag, "signal SIGBUS caught ");
            oss<<"signal SIGBUS caught ";

            switch (info->si_code) {
                case BUS_ADRALN:
                    LOGI(kTag, "invalid address alignment");
                    oss<<"invalid address alignment ";

                    break;
                case BUS_ADRERR:
                    LOGI(kTag, "nonexistent physical address");
                    oss<<"nonexistent physical address ";

                    break;
                case BUS_OBJERR:
                    LOGI(kTag, "object-specific hardware error");
                    oss<<"object-specific hardware error ";
                    break;
                default:
                    LOGI(kTag, "code = %d", info->si_code);
                    break;
            }
            break;
        case SIGABRT:
            oss<<"signal SIGABRT caught ";

            LOGI(kTag, "signal SIGABRT caught");

            break;
        case SIGPIPE:
            LOGI(kTag, "signal SIGPIPE caught");
            oss<<"signal SIGPIPE caught ";

            break;
        default:
            LOGI(kTag, "signo %d caught", info->si_signo);
            LOGI(kTag, "code = %d", info->si_code);
    }
    LOGI(kTag, "errno = %d", info->si_errno);
    writeStringToFIle(oss.str());
}

static void DumpStacks(void *context) {
    LOGI(kTag, "into --DumpStacks");
    sTidToDump = gettid();
    sContext = context;
    StackDumpingThread();

}


static void StackDumpingThread() {
    class Callback : public GetTraceCallback {
    public:
        void OnFrame(size_t frame_num, std::string frame) override {

           writeStringToFIle(frame);

        }

        void OnFail() override {
            LOGW(kTag, "Fail to get stack trace");
        }
    };
    std::unique_ptr<Callback> callback{new Callback};
    GetStackTrace(sTidToDump, sContext, callback.get());
    LOGW(kTag, "释放线程锁");
}

