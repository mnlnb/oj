#include <iostream>
#include <signal.h>

#include "../comm/httplib.h"
#include "oj_control.hpp"

using namespace httplib;
using namespace ns_control;

// HTTP 路由共享一个 Control。信号回调通过这个指针访问同一个实例。
static Control *ctrl_ptr = nullptr;

// 收到 SIGQUIT 时调用恢复入口，把已标为离线的编译节点移回候选列表。这里没有先探测节点。
void Recovery(int signo)
{
    ctrl_ptr->RecoveryMachine();
}

int main()
{
    signal(SIGQUIT, Recovery);

    //用户请求的服务路由功能
    Server svr;

    Control ctrl;
    ctrl_ptr = &ctrl;

    // 获取所有的题目列表
    // 题目列表由 Model 读取题库，Control 按数字题号排序，View 再填充列表模板。
    svr.Get("/all_questions", [&ctrl](const Request &req, Response &resp){
        //返回一张包含有所有题目的html网页
        std::string html;
        ctrl.AllQuestions(&html);
        //用户看到的是什么呢？？网页数据 + 拼上了题目相关的数据
        resp.set_content(html, "text/html; charset=utf-8");
    });

    // 用户要根据题目编号，获取题目的内容
    // /question/100 -> 正则匹配
    // R"()", 原始字符串raw string,保持字符串内容的原貌，不用做相关的转义
    // 正则捕获组匹配数字题号。详情页包含题面和编辑器初始代码，测试尾代码留在后端。
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp){
        std::string number = req.matches[1];
        std::string html;
        ctrl.Question(number, &html);
        resp.set_content(html, "text/html; charset=utf-8");
    });

    // 用户提交代码，使用我们的判题功能(1. 每道题的测试用例 2. compile_and_run)
    // 题号来自 URL，代码来自请求正文。控制器把提交和题目测试用例组装后交给编译节点。
    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request &req, Response &resp){
        std::string number = req.matches[1];
        std::string result_json;
        ctrl.Judge(number, req.body, &result_json);
        resp.set_content(result_json, "application/json;charset=utf-8");
        // resp.set_content("指定题目的判题: " + number, "text/plain; charset=utf-8");
    });
    
    // 静态首页来自当前工作目录下的 wwwroot。conf 和 template_html 也按相对路径读取。
    svr.set_base_dir("./wwwroot");
    if (!svr.listen("127.0.0.1", 18080)) {
        std::cerr << "Could not listen on 127.0.0.1:18080\n";
        return 1;
    }
    return 0;
}