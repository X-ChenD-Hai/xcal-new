#include <format>
#include <print>
#include <process.hpp>
#include <string>
#include <vector>

int main() {
    using namespace TinyProcessLib;
    int width = 800;
    int height = 600;
    int fps = 30;
    std::string output_file = "output.mp4";
    std::vector<std::string> commands = {
        "ffmpeg",
        "-y",                                              // 覆盖输出
        "-f",        "rawvideo",                           // 输入格式: 原始视频
        "-s",        std::format("{}x{}", width, height),  // 帧尺寸
        "-pix_fmt",  "rgba",                               // 像素格式
        "-r",        std::to_string(fps),                  // 帧率
        "-i",        "-",                                  // 从 stdin 读取
        "-vf",       "vflip",  // 垂直翻转 (OpenGL vs 视频坐标系)
        "-an",                 // 无音频
        "-loglevel", "error",
        "-vcodec",   "libx264",  // H.264
        "-pix_fmt",  "yuv420p",  // 兼容性像素格式
        output_file};

    Process process(
        commands, "",
        [](const char* s, size_t n) {
            std::string output(s, n - 1);
            std::println("Process output: {}", output);
        },
        [](const char* s, size_t n) {
            std::string error(s, n);
            std::println("Process error: {}", error);
        },
        true);

    for (int i = 0; i < fps * 5; ++i) {  // 5秒的视频
        std::vector<uint8_t> frame_data(width * height * 4, 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                size_t idx = (y * width + x) * 4;
                frame_data[idx] = static_cast<uint8_t>((x + i) % 256);      // R
                frame_data[idx + 1] = static_cast<uint8_t>((y + i) % 256);  // G
                frame_data[idx + 2] =
                    static_cast<uint8_t>((x + y + i) % 256);  // B
                frame_data[idx + 3] = 255;                    // A
            }
        }
        process.write((char*)frame_data.data(),
                      frame_data.size() * sizeof(uint8_t));
    }
    process.close_stdin();  // 关闭 stdin，通知 ffmpeg 输入结束

    // process.write("Hello, subprocess!\n");

    // std::println("Process ID: {}", process.get_id());
    int exit_status = process.get_exit_status();
    std::println("Process exit status: {}", exit_status);

    return 0;
}