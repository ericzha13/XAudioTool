// test_win.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define GLOG_NO_ABBREVIATED_SEVERITIES 0
#include <iostream>
#include "TimeHelp.hpp"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "AudioAssistant.h"

DEFINE_string(input_file, "", "Input file path");
DEFINE_int32(iterations, 100, "Number of iterations");
DEFINE_bool(verbose, false, "Enable verbose mode");



int main(int argc, char** argv)
{
    //puts(argv[0]);

	google::InitGoogleLogging("AABB");

	//google::SetLogDestination(google::GLOG_INFO,R"(D:\Var\Xaudio\agv_4)");
	//google::SetLogFilenameExtension(".log");



	FLAGS_v = 1;// no used
	//FLAGS_logtostdout = true;
	//FLAGS_alsologtostderr = true;
	FLAGS_logtostderr = true;
	// FLAGS_stderrthreshold = google::GLOG_WARNING;
	FLAGS_colorlogtostdout = true;
	FLAGS_colorlogtostderr = true;
	//FLAGS_max_log_size = 3;
	 //FLAGS_stop_logging_if_full_disk = true;



	LOG(INFO) << "start time now :" << ast::utils::get_format_time();
    {
        std::cout << "Hello World!\n";
        scope_delay_ms(1000);

		
		try
		{
			std::filesystem::path tt_folder = R"(C:\Users\ericzha\Music\TestData\vw_log_2023-04-28_09_44_49.649630_final0.pcm)";
			MergeAudio aa(tt_folder);
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << "std::exception " << e.what();
		}
		
        
        
    }
	LOG(INFO) << "End time now :" << ast::utils::get_format_time() << std::endl;
    



    google::ShutdownGoogleLogging();

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
