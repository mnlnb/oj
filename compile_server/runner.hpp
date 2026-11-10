#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../comm/log.hpp"
#include "../comm/util.hpp"

namespace ns_runner
{
    using namespace ns_util;
    using namespace ns_log;

    class Runner
    {
    public:
        Runner() {}
        ~Runner() {}

    public:
        //提供设置进程占用资源大小的接口
        // RLIMIT_CPU 限制进程消耗的 CPU 时间，单位为秒。RLIMIT_AS 限制虚拟地址空间，单位为字节。
        // 这里只设置资源软上限，不能代替墙钟超时或文件、网络和进程权限隔离。
        static void SetProcLimit(int _cpu_limit, int _mem_limit)
        {
            // 设置CPU时长
            struct rlimit cpu_rlimit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            cpu_rlimit.rlim_cur = _cpu_limit;
            setrlimit(RLIMIT_CPU, &cpu_rlimit);

            // 设置内存大小
            struct rlimit mem_rlimit;
            mem_rlimit.rlim_max = RLIM_INFINITY;
            mem_rlimit.rlim_cur = _mem_limit * 1024; // 将配置值从 KB 换算成字节
            setrlimit(RLIMIT_AS, &mem_rlimit);
        }
        // 指明文件名即可，不需要代理路径，不需要带后缀
        /*******************************************
         * 返回值 > 0: 程序异常了，退出时收到了信号，返回值就是对应的信号编号
         * 返回值 == 0: 未观察到终止信号，输出保存到临时文件中。
         * 返回值 < 0: 内部错误
         * 
         * cpu_limit: 该程序运行的时候，可以使用的最大cpu资源上限
         * mem_limit: 改程序运行的时候，可以使用的最大的内存大小(KB)
         * *****************************************/
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            /*********************************************
             * 程序运行：
             * 1. 代码跑完，结果正确
             * 2. 代码跑完，结果不正确
             * 3. 代码没跑完，异常了
             * Run需要考虑代码跑完，结果正确与否吗？？不考虑！
             * 结果正确与否：是由我们的测试用例决定的！
             * 我们只考虑：是否正确运行完毕
             *
             * 我们必须知道可执行程序是谁？
             * 一个程序在默认启动的时候
             * 标准输入: 不处理
             * 标准输出: 程序运行完成，输出结果是什么
             * 标准错误: 运行时错误信息
             * *******************************************/
            // 为本次任务打开三份文件。stdin 当前是空文件，stdout 和 stderr 留给上层分别读取。
            std::string _execute = PathUtil::Exe(file_name);
            std::string _stdin   = PathUtil::Stdin(file_name);
            std::string _stdout  = PathUtil::Stdout(file_name);
            std::string _stderr  = PathUtil::Stderr(file_name);

            umask(0);
            int _stdin_fd = open(_stdin.c_str(), O_CREAT|O_RDONLY, 0644);
            int _stdout_fd = open(_stdout.c_str(), O_CREAT|O_WRONLY, 0644);
            int _stderr_fd = open(_stderr.c_str(), O_CREAT|O_WRONLY, 0644);

            if(_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0){
                LOG(ERROR) << "运行时打开标准文件失败" << "\n";
                return -1; //代表打开文件失败
            }            

            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "运行时创建子进程失败" << "\n";
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                return -2; //代表创建子进程失败
            }
            else if (pid == 0)
            {
                // 子进程把标准输入、标准输出和标准错误分别接到文件描述符 0、1、2。
                // 随后 execl 启动提交程序，重定向和资源限制会继续生效。
                dup2(_stdin_fd, 0);
                dup2(_stdout_fd, 1);
                dup2(_stderr_fd, 2);

                SetProcLimit(cpu_limit, mem_limit);
                
                execl(_execute.c_str()/*我要执行谁*/, _execute.c_str()/*我想在命令行上如何执行该程序*/, nullptr);
                exit(1);
            }
            else
            {
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                int status = 0;
                // waitpid 返回编码后的子进程状态。当前只取低 7 位作为信号编号，未检查程序的普通退出码。
                // 因此返回 0 只代表未观察到终止信号，不能据此判断测试通过。
                waitpid(pid, &status, 0);
                // 这里返回信号编号。普通退出码位于 waitpid 状态的其他位。
                LOG(INFO) << "运行完毕, info: " << (status & 0x7F) << "\n"; 
                return status & 0x7F;
            }
        }
    };
}