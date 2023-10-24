#pragma once


#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include "windows.h"
#include <sys\stat.h>
#include <fstream>
#include <conio.h>
#include <algorithm>
#include <stdexcept>
#include <climits>

#include "json.hpp"


namespace fs = std::filesystem;

//��ĳһ���ļ�
#define OPEN_ONE_FILE(m_ofs, filename) \
    { \
        m_ofs.open(filename, std::ios::binary | std::ios::out); \
        if (!m_ofs.is_open()) { \
            std::cerr << "output audio file open failed " << filename << std::endl; \
            return false; \
        } else { \
            std::cout << "output = " << filename << std::endl; \
        } \
    }


//fileHelp
namespace ast::utils
{

	//�ҳ�·����ָ����׺���ļ��������ڣ������
	//����ж������������path����string?
	//str:��Ѱ���ļ��ĺ�׺��,���Ҫ��ȡ�����ļ�������*
	template <typename T>
	inline bool get_files_from_directory(const T& input_path, const char* ext_name, std::vector<fs::path>& v_output)
	{
		if constexpr (!std::is_constructible_v<fs::path, T>)
		{
			static_assert(std::is_constructible_v<fs::path, T>, "Only string and path types are supported.");//����׶��׳�
			return false;
		}

		const auto path = fs::path(input_path);

		if (!fs::is_directory(path))
		{
			//std::cerr << "The input is not a valid directory,may be not exist" << std::endl;
			return false;
		}

		for (auto& itr : fs::directory_iterator(path))
		{
			if (itr.is_regular_file() && (itr.path().extension() == ext_name || ext_name == "*"))
			{
				v_output.emplace_back(itr);
			}
		}

		std::sort(v_output.begin(), v_output.end());
		return true;
	}

	bool FindFilesWithExtension(const fs::path& folder_path, const std::string& ext_name, std::vector<fs::path>& v_output,bool recursive = false) {
		if (!fs::is_directory(folder_path)) {
			//std::cerr << "The input argu is not a valid directory or may not exist: " << folder_path << std::endl;
			return false;
		}

		for (const auto& entry : fs::directory_iterator(folder_path)) {
			if (entry.is_regular_file() && (entry.path().extension() == ext_name || ext_name == "*")) {
				v_output.emplace_back(entry);
			}
			else if (entry.is_directory() && recursive) {
				FindFilesWithExtension(entry, ext_name, v_output, recursive); // �ݹ���ã��������ļ���
			}
		}

		return true;
	}

	const std::string_view windows_invalid_chars = R"(""*:<>?/|)";
	const std::string_view allowed_characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";

	inline bool is_valid_name(const std::string_view& name)
	{
		const bool has_valid_length = !name.empty() && name.front() != ' ' && name.back() != ' ';
		const bool has_invalid_chars = name.find_first_of(windows_invalid_chars) != std::string_view::npos;
		const bool has_allowed_chars = name.find_first_not_of(allowed_characters) == std::string_view::npos;
		const bool is_dot_or_dot_dot = name == "." || name == "..";

		return has_valid_length && !has_invalid_chars && has_allowed_chars && !is_dot_or_dot_dot;
	}


	/*
	�𼶴���Ŀ¼,�����ƣ�����ʹ��Tģ��
	
	ʹ�÷���:create_format_directory(variable,"directory1","directory2","directory3")��

	�������true�������һ��������ʾ�����ļ���Ĭ��false:���һ��������ʾ�����ļ���
	ȱ�ݣ�if���̫�ࡣ
	*/
	template <bool is_file = false, typename... Args >
	std::string create_format_directory(const fs::path& original_path, const std::string&& first, Args&&... rest)
	{
		if (!fs::is_directory(original_path) || !is_valid_name(first)) {
			std::cerr << "original directory" << original_path << "  not exist, or create failed or[" << first << "] is invalid" << std::endl;
			return "";
		}

		const auto & tmp_path = fs::path(original_path) / first;

		if constexpr (sizeof...(Args) == 0) {//�ݹ����
			if (is_file == true) {
				std::ofstream m_ofs_tmp(tmp_path, std::ios_base::trunc | std::ios_base::out);
				if (m_ofs_tmp) { m_ofs_tmp.close(); return (tmp_path).string(); }
			}
			else if (fs::exists(tmp_path) || fs::create_directory(tmp_path)) {
				return tmp_path.string();
			}
			return "";
		}
		else {
			fs::create_directory(tmp_path);
			return create_format_directory<is_file>(tmp_path, std::forward<Args>(rest)...);
		}
	}
	

	/*
		����һ��vector��������ļ�·����
		�����һ��vector,��Ŷ�Ӧ��Щ�ļ����ļ����
	*/
	inline long open_files(const std::vector<fs::path>& files, std::vector<std::ifstream>& file_handles)
	{
		const auto file_count = files.size();
		//long long minimum_file_size = LLONG_MAX; // ��С�ļ���С�ĳ�ʼֵ
	
		// ʹ�� emplace_back ������� ifstream ���� vector ������
		for (std::size_t i = 0; i < file_count; ++i) {
			file_handles.emplace_back(std::ifstream(files.at(i), std::ios::binary | std::ios_base::in | std::ios_base::ate));
			// ����ļ���ʧ�ܣ����¼������Ϣ������ false
			if (!file_handles.at(i).is_open()) {
				std::cerr << files.at(i) << " ��ʧ�ܣ�" << std::endl;
				return -1;
			}
			// ������С�ļ���С�������ļ�ָ��
			//minimum_file_size = std::min(minimum_file_size, static_cast<long long>(file_handles[i]->tellg()));
			file_handles.at(i).seekg(0, std::ios_base::beg);
		}
	
		//std::cout << "minimum file size: " << minimum_file_size << std::endl;
		//return minimum_file_size;
		return 0;
	}

	inline long open_files(const std::vector<fs::path>& files, std::vector<std::ofstream>& file_handles)
	{
		const auto file_count = files.size();
		//long long minimum_file_size = LLONG_MAX; // ��С�ļ���С�ĳ�ʼֵ
	
		// ʹ�� emplace_back ������� ifstream ���� vector ������
		for (std::size_t i = 0; i < file_count; ++i) {
			file_handles.emplace_back(std::ofstream(files.at(i), std::ios::binary | std::ios_base::out));
			// ����ļ���ʧ�ܣ����¼������Ϣ������ false
			if (!file_handles.at(i).is_open()) {
				std::cerr << files.at(i) << " ��ʧ�ܣ�" << std::endl;
				return -1;
			}
		}
	
		//std::cout << "minimum file size: " << minimum_file_size << std::endl;
		//return minimum_file_size;
		return 0;
	}



	//����һ��vector���ж������ļ��Ƿ��������ļ�
	template<typename T>
	bool are_files_regular(const std::vector<T>& container) {
		if constexpr (std::is_constructible_v<fs::path, T>) {
			for (const auto& item : container) {
				if (!fs::is_regular_file(item)) return false;
			}
		}
		else {
			std::cout << "Invalid container type. Not processing." << std::endl;
			return false;
		}
		return true;
	}

}


