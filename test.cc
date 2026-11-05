#include <iostream>
#include <string>
#include <ctemplate/template.h>

int main()
{
    std::string in_html = "./test.html";
    std::string value = "比特就业课";

    // 形成数据字典
    ctemplate::TemplateDictionary root("test"); //unordered_map<> test;
    root.SetValue("key", value);                //test.insert({});

    // 获取被渲染网页对象
    ctemplate::Template *tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);

    // 添加字典数据到网页中
    std::string out_html;
    tpl->Expand(&out_html, &root);

    //完成了渲染
    std::cout << out_html << std::endl;
}





























// #include <iostream>
// #include <vector>
// #include <boost/algorithm/string.hpp>

// // #include <sys/time.h>
// // #include <sys/resource.h>
// // #include <unistd.h>
// // #include <signal.h>
// // #include <jsoncpp/json/json.h>

// int main()
// {
//     std::vector<std::string> tokens;
//     const std::string str = "1:判断回文数::::::简单:1:30000";
//     const std::string sep = ":";
//     boost::split(tokens, str, boost::is_any_of(sep), boost::algorithm::token_compress_on);

//     for(auto &iter : tokens){
//         std::cout << iter << std::endl;
//     }

//     // 序列化的工作
//     //将结构化数据转化成为一个字符串
//     //Value是一个Json的中间类，可以填充KV值
//     // Json::Value root;
//     // root["code"] = "mycode";
//     // root["user"] = "whb";
//     // root["age"] = "19";

//     // // Json::StyledWriter writer;
//     // Json::FastWriter writer;
//     // std::string str = writer.write(root);
//     // std::cout << str << std::endl;
// }

// // void handler(int signo)
// // {
// //     std::cout << "signo : " << signo << std::endl;
// //     exit(1);
// // }

// // int main()
// // {
// //     //资源不足，导致OS终止进程，是通过信号终止的
// //     for(int i =1; i <= 31; i++){
// //         signal(i, handler);
// //     }

// //     // 限制累计运行时长
// //     struct rlimit r;
// //     r.rlim_cur = 1;
// //     r.rlim_max = RLIM_INFINITY;
// //     setrlimit(RLIMIT_CPU, &r);
// //     while(1);

// //     // struct rlimit r;
// //     // r.rlim_cur = 1024 * 1024 * 40; //20M
// //     // r.rlim_max = RLIM_INFINITY;
// //     // setrlimit(RLIMIT_AS, &r);

// //     // int count = 0;
// //     // while(true)
// //     // {
// //     //     int *p = new int[1024*1024];
// //     //     count++;
// //     //     std::cout << "size: " << count << std::endl;
// //     //     sleep(1);
// //     // }
// //     return 0;
// // }