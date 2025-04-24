// #include <iostream>
// #include <emscripten.h>
// #include "dng_sdk_1_7/dng_sdk/source/dng_file_stream.h"
// #include "dng_sdk_1_7/dng_sdk/source/dng_info.h"
// #include "dng_sdk_1_7/dng_sdk/source/dng_host.h"
// #include "dng_sdk_1_7/dng_sdk/source/dng_negative.h"
// #include <stdlib.h>
//
// extern "C" {
//     EMSCRIPTEN_KEEPALIVE
//
//     int getInitWbPerFile(const char* path) {
//         try {
//             std::cout<<"path is "<< std::string(path)   << std::endl;
//             std::cout << "defining dng host " << std::endl;
//             dng_host h;
//             std::cout << "defining dng stream " << std::endl;
//             dng_file_stream stream(path);
//             std::cout << "defining dng info " << std::endl;
//
//             dng_info info;
//             info.Parse(h, stream);
//             info.PostParse(h);
//
//             if (!info.IsValidDNG()) {
//                 return -1; // Error code for invalid DNG
//             }
//
//             std::unique_ptr<dng_negative> negative(h.Make_dng_negative());
//             negative->Parse(h, stream, info);
//             negative->PostParse(h, stream, info);
//
//             // TODO: Implement your actual white balance calculation here
//             return 1;
//         } catch (const dng_exception &e) {
//             std::cout<< std::string(path) << "🚀 dng exception is is " << e.ErrorCode()  << std::endl;
//
//             return -3; // Error code for general exception
//         } catch (std::exception& e) {
//             std::cout << "🚀 exception is is " << e.what() << std::endl;
//
//             return -1; // Error code for general exception
//         } catch (...) {
//             return -2; // Error code for general exception
//         }
//     }
// }
//
// int main() {
//     std::cout << "🚀 Hello from C++ running in the browser!" << std::endl;
//     const char* p = "C:\\Users\\hadard\\Downloads\\wasm_test_image.dng";
//     int result = getInitWbPerFile(p);
//     std::cout << "🚀 result is " << result << std::endl;
//
//     return 0;
// }
//
#include <cstdio>
#include <dng_color_spec.h>
#include <dng_file_stream.h>
#include <dng_host.h>
#include <dng_info.h>
#include <dng_render.h>
#include <dng_temperature.h>
#include <string>
#include <iostream>
#include <fstream>
#include <emscripten/bind.h>

#include <iostream>
#include <fstream>
#include <iomanip>
//#include <filesystem>

//namespace fs = std::filesystem;

using namespace emscripten;
template <typename T>
T clamp(T val, T min_val, T max_val) {
    return std::max(min_val, std::min(val, max_val));
}
static dng_xy_coord calculateDefaultWhiteBalance(const std::unique_ptr<dng_negative>& neg, dng_host& host) {
    dng_render negRender(host, *neg);
    //based on dng_image_writer color_tag_set::color_tag_set
    if (neg->HasCameraWhiteXY ()){
        return neg->CameraWhiteXY();
    }

    //based on dng_render - dng_render_task::Start
    else if (neg->HasCameraNeutral ()) {
        auto spec = neg->MakeColorSpec(negRender.CameraProfileID ());
        return spec->NeutralToXY(neg->CameraNeutral());
    }
}

static int extractInitialTemperatureAndTint(dng_host& host, std::unique_ptr<dng_negative>&neg) {
    dng_xy_coord wbPoint = calculateDefaultWhiteBalance(neg, host);
    dng_temperature temperatureFromXy(wbPoint);
    int xy_to_tint = std::lround(temperatureFromXy.Tint());

    auto xy_to_temp = std::round(temperatureFromXy.Temperature() / 50.0) * 50.0;
    xy_to_temp = clamp(xy_to_temp, 2000.0, 50000.0);
    // xy_to_temp = std::clamp(xy_to_temp, 2000.0, 50000.0); //only in c++17

    std::cout << "temp " <<  static_cast<int>(xy_to_temp) << "\n";
    std::cout << "tint " <<  xy_to_tint  << "\n";

    return 1;
}


// static int getInitWbPerFile(const std::fs::path& path) {
static int getInitWbPerFile(const std::string& path) {
    std::cout << "getInitWbPerFile " << path << "\n";

    dng_host h;
    dng_file_stream stream(fopen(path.c_str(), "r"));

    // dng file
    dng_info info;

    info.Parse(h, stream);
    info.PostParse(h);
    if (!info.IsValidDNG()) {
        std::cout << "INVALID DNG!!"  << "\n";
        // throw ImagentException("DngUtils::getInitialWhiteBalance failed to pars dng file " + utils::pathToStr(path) , IMAGENT_ERROR::IMAGENT_UNKNOWN_EXCEPTION);
    }

    std::unique_ptr<dng_negative> negative(h.Make_dng_negative());

    negative->Parse(h, stream, info);
    negative->PostParse(h, stream, info);

    extractInitialTemperatureAndTint(h, negative);
    return 1;
}



std::string read_file(const std::string &fn)
{

    std::cout << "read_file " << fn << "\n";
    try {
        getInitWbPerFile(fn);

    }
    catch (const std::exception &e) {
        std::cout << "exception caught in  getInitWbPerFile" << e.what() << "\n";

    }
    // std::ifstream f(fn);
    // std::string line;
    // std::getline(f, line);
    // std::cout << "line " << line << "\n";
    // return line;
    return "finish reading files!";
}

EMSCRIPTEN_BINDINGS(hello) {
    function("read_file", &read_file);
}