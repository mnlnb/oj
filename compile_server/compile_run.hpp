#pragma once

#include "compiler.hpp"
#include "runner.hpp"
#include "../comm/log.hpp"
#include "../comm/util.hpp"

#include <signal.h>
#include <unistd.h>
#include <json/json.h>

namespace ns_compile_and_run
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_compiler;
    using namespace ns_runner;

    // 组织一次编译任务的完整生命周期。HTTP 层只负责收发 JSON，Compiler 和 Runner 分别处理两个子进程。
    class CompileAndRun
    {
    public:
        // 正常返回路径统一清理本次任务涉及的文件。编译失败时通常只有源码和编译诊断文件。
        static void RemoveTempFile(const std::string &file_name)
        {
            //清理文件的个数是不确定的，但是有哪些我们是知道的
            std::string _src = PathUtil::Src(file_name);
            if(FileUtil::IsFileExists(_src)) unlink(_src.c_str());

            std::string _compiler_error = PathUtil::CompilerError(file_name);
            if(FileUtil::IsFileExists(_compiler_error)) unlink(_compiler_error.c_str());

            std::string _execute = PathUtil::Exe(file_name);
            if(FileUtil::IsFileExists(_execute)) unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if(FileUtil::IsFileExists(_stdin)) unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if(FileUtil::IsFileExists(_stdout)) unlink(_stdout.c_str());

            std::string _stderr = PathUtil::Stderr(file_name);
            if(FileUtil::IsFileExists(_stderr)) unlink(_stderr.c_str());
        }
        // code > 0 : 进程收到了信号导致异常奔溃
        // code < 0 : 整个过程非运行报错(代码为空，编译报错等)
        // code = 0 : 整个过程全部完成
        //待完善
        // 负数是服务自定义状态，正数来自运行子进程的信号编号。编译错误详情从诊断文件读取。
        // 对信号的文案是现有约定，同一个信号也可能由其他原因触发。
        static std::string CodeToDesc(int code, const std::string &file_name)
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "编译运行成功";
                break;
            case -1:
                desc = "提交的代码是空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                // desc = "代码编译的时候发生了错误";
                FileUtil::ReadFile(PathUtil::CompilerError(file_name), &desc, true);
                break;
            case SIGABRT: // 6
                desc = "内存超过范围";
                break;
            case SIGXCPU: // 24
                desc = "CPU使用超时";
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";
                break;
            default:
                desc = "未知: " + std::to_string(code);
                break;
            }

            return desc;
        }

        /***************************************
         * 输入:
         * code： 用户提交的代码
         * input: 请求预留的标准输入字段，当前没有写入子进程的输入文件。
         * cpu_limit: CPU 时间软上限，单位为秒。
         * mem_limit: 虚拟地址空间软上限，单位为 KB。
         *
         * 输出:
         * 必填
         * status: 状态码
         * reason: 请求结果
         * 选填：
         * stdout: 我的程序运行完的结果
         * stderr: 我的程序运行完的错误结果
         *
         * 参数：
         * in_json: {"code": "#include...", "input": "","cpu_limit":1, "mem_limit":10240}
         * out_json: {"status":"0", "reason":"","stdout":"","stderr":"",}
         * ************************************/
        static void Start(const std::string &in_json, std::string *out_json)
        {
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value); //最后在处理差错问题

            std::string code = in_value["code"].asString();
            // 当前只从请求中读取 input，后续没有写入标准输入文件。实际子进程读到的是空文件。
            std::string input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            int status_code = 0;
            Json::Value out_value;
            int run_result = 0;
            std::string file_name; //需要内部形成的唯一文件名

            if (code.size() == 0)
            {
                status_code = -1; //代码为空
                goto END;
            }
            // 任务名本身不含目录和后缀，各阶段通过 PathUtil 生成完整路径。
            file_name = FileUtil::UniqFileName();
            //形成临时src文件
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code))
            {
                status_code = -2; //未知错误
                goto END;
            }

            // 只有找到 g++ 生成的可执行文件才继续运行。编译器诊断稍后会填入 reason。
            if (!Compiler::Compile(file_name))
            {
                //编译失败
                status_code = -3; //代码编译的时候发生了错误
                goto END;
            }

            // 题库给出的 CPU 秒数和内存 KB 会传给 Runner，在运行提交代码的子进程中应用。
            run_result = Runner::Run(file_name, cpu_limit, mem_limit);
            if (run_result < 0)
            {
                status_code = -2; //未知错误
            }
            else if (run_result > 0)
            {
                //程序运行崩溃了
                status_code = run_result;
            }
            else
            {
                //运行成功
                status_code = 0;
            }
        // 各分支在这里汇合。先读取仍在磁盘上的诊断或输出，再生成响应，最后删除临时文件。
        END:
            out_value["status"] = status_code;
            out_value["reason"] = CodeToDesc(status_code, file_name);
            // status 为 0 只表示当前编译和运行流程未报告错误。测试通过与否看测试代码写到 stdout 的内容。
            if (status_code == 0)
            {
                // 整个过程全部成功
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                out_value["stdout"] = _stdout;

                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
                out_value["stderr"] = _stderr;
            }

            Json::StyledWriter writer;
            *out_json = writer.write(out_value);

            RemoveTempFile(file_name);
        }
    };
}
