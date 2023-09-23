// test_win.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "TimeHelp.hpp"
#include <gflags/gflags.h>

DEFINE_string(input_file, "", "Input file path");
DEFINE_int32(iterations, 100, "Number of iterations");
DEFINE_bool(verbose, false, "Enable verbose mode");

#define STRIP_FLAG_HELP 1
using GFLAGS_NAMESPACE::SetUsageMessage;
using GFLAGS_NAMESPACE::ParseCommandLineFlags;
int main(int argc, char** argv)
{
    SetUsageMessage("Usage message");
    ParseCommandLineFlags(&argc, &argv, false);
    puts(argv[0]);

    std::cout << "start time now :" << ast::utils::get_format_time() << std::endl;
    
    {
        std::cout << "Hello World!\n";
        scope_delay_ms(300);
        
        long res = 0;
        for (int i = 0; i < 100; i++) {
            res += i;
        }

        {
            // Initialize gflags.
            gflags::ParseCommandLineFlags(&argc, &argv, true);
         
            // Access command-line parameters.
            std::string input_file = FLAGS_input_file;
            int iterations = FLAGS_iterations;
            bool verbose = FLAGS_verbose;
         
            // Print the parsed values.
            std::cout << "Input File: " << input_file << std::endl;
            std::cout << "Iterations: " << iterations << std::endl;
            std::cout << "Verbose Mode: " << (verbose ? "true" : "false") << std::endl;

        }

        std::cout << "res = " << res << std::endl;
    }
    std::cout << "End time now :" << ast::utils::get_format_time() << std::endl;

}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
