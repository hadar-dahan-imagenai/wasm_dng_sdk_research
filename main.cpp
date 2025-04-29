
#include <cassert>
#include <cstdio>
#include <dng_color_space.h>
#include <dng_color_spec.h>
#include <dng_file_stream.h>
#include <dng_host.h>
#include <dng_image_writer.h>
#include <dng_info.h>
#include <dng_memory_stream.h>
#include <dng_preview.h>
#include <dng_render.h>
#include <dng_tag_codes.h>
#include <dng_temperature.h>
#include <dng_xmp.h>
#include <dng_xmp_sdk.h>
#include <string>
#include <iostream>
#include <fstream>
#include <emscripten/bind.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_map>
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
    return dng_xy_coord{};
}

static int extractInitialTemperatureAndTint(dng_host& host, std::unique_ptr<dng_negative>&neg) {
    dng_xy_coord wbPoint = calculateDefaultWhiteBalance(neg, host);
    dng_temperature temperatureFromXy(wbPoint);
    int xy_to_tint = std::lround(temperatureFromXy.Tint());

    auto xy_to_temp = std::round(temperatureFromXy.Temperature() / 50.0) * 50.0;
    xy_to_temp = clamp(xy_to_temp, 2000.0, 50000.0);
    // xy_to_temp = std::clamp(xy_to_temp, 2000.0, 50000.0); //only in c++17

    // std::cout << "temp " <<  static_cast<int>(xy_to_temp) << "\n";
    // std::cout << "tint " <<  xy_to_tint  << "\n";

    return 1;
}

void writeTiffTemplate(const std::string& outFilename, std::unordered_map<std::string, real64>& editingParamsMap, dng_host& host, std::unique_ptr<dng_negative>&neg, std::vector<uint8>& vec) {
    // std::cout << "writeTiffTemplate strat " << std::endl;

    dng_render negRender(host, *neg);


    // negRender.SetFinalPixelType(ttShort); //need this???

    std::vector<int> initTemp(2);
    // if (!editingParamsMap.contains("temp") || !editingParamsMap.contains("tint")) {
    //     initTemp = getInitialTemperature(); // Call getInitialTemperature() only if missing values
    // }
    // Get temperature and tint from the map, or use default values if not found
    // editingParamsMap["temp"] = editingParamsMap.contains("temp") ? editingParamsMap.at("temp") : initTemp[0];
    // editingParamsMap["tint"] = editingParamsMap.contains("tint") ? editingParamsMap.at("tint") : initTemp[1];

    // Create the temperature object and set the white balance
    dng_temperature dngTemperature(editingParamsMap["temp"], editingParamsMap["tint"]);
    negRender.SetWhiteXY(dngTemperature.Get_xy_coord());

    if (editingParamsMap.find("exposure") != editingParamsMap.end()) {
        negRender.SetExposure(editingParamsMap.at("exposure"));
    }
    if (editingParamsMap.find("shadows") != editingParamsMap.end()) {
        negRender.SetShadows(editingParamsMap.at("shadows"));
    }

    AutoPtr<dng_image> negImage(negRender.Render());

    dng_memory_stream stream (host.Allocator ());

    try {
        AutoPtr<dng_jpeg_preview> jpeg(new dng_jpeg_preview());
        dng_string appNameVersion("mini-lr-poc"); appNameVersion.Append(" "); appNameVersion.Append("mini-lr-0.1");
        // dng_string appNameVersion(m_appName); appNameVersion.Append(" "); appNameVersion.Append(m_appVersion.Get());
        dng_date_time_info m_dateTimeNow;
        CurrentDateTimeAndZone(m_dateTimeNow);

        jpeg->fInfo.fApplicationName.Set_ASCII(appNameVersion.Get());
        jpeg->fInfo.fApplicationVersion.Set_ASCII(appNameVersion.Get());
        jpeg->fInfo.fDateTime = m_dateTimeNow.Encode_ISO_8601();
        jpeg->fInfo.fColorSpace = previewColorSpace_sRGB;

        dng_image_writer jpegWriter; jpegWriter.EncodeJPEGPreview(host, *negImage.Get(), *jpeg.Get(), 8);

        auto& block = jpeg->CompressedData();
        auto buf = jpeg->CompressedData().Buffer_uint8();
        size_t size = block.LogicalSize();

        std::vector<uint8> new_vec(buf, buf + size);
        vec = new_vec;
        // return vec;
    }
    catch (dng_exception& e) {
       std::cout << "Error while writing TIFF-file! " << e.ErrorCode() << std::endl;
    }
}
void renderImage(std::unique_ptr<dng_negative>&neg, dng_host &host) {

    try {
        neg->SynchronizeMetadata();

        neg->BuildStage2Image(host);   // Compute linearized and range-mapped image
        neg->BuildStage3Image(host);   // Compute demosaiced image (used by preview and thumbnail)
    }
    catch (dng_exception& e) {
        // std::stringstream error; error << "Error while rendering image from raw! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error("Error while rendering image from raw!");
//        throw std::stdruntime_error(error.str());
    }
}
void dngProcessor(const std::string &path, dng_host &host, std::unique_ptr<dng_negative>&neg) {
    // Re-read source DNG using DNG SDK - we're ignoring the LibRaw/Exiv2 data structures from now on
    try {
        dng_file_stream stream(path.c_str());
        dng_info info;
        info.Parse(host, stream);
        info.PostParse(host);
        if (!info.IsValidDNG()) throw dng_exception(dng_error_bad_format);

        // neg.reset(host.Make_dng_negative());//init neg after host is ready??

        neg->Parse(host, stream, info);
        neg->PostParse(host, stream, info);
        neg->ReadStage1Image(host, stream, info);
        neg->ReadTransparencyMask(host, stream, info);
        neg->ValidateRawImageDigest(host);
    }
    catch (const dng_exception &except) {throw except;}
    catch (...) {throw dng_exception(dng_error_unknown);}
}

// static int getInitWbPerFile(const std::fs::path& path) {
static int getInitWbPerFile(const std::string& path) {
    // std::cout << "getInitWbPerFile " << path << "\n";

    dng_host h;
    dng_file_stream stream(path.c_str(), false, dng_stream::kBigBufferSize);

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


void setCameraProfileFile(const std::string &dcpFilename, bool customIfDcpMissing, std::unique_ptr<dng_negative>&neg) {
    AutoPtr<dng_camera_profile> prof(new dng_camera_profile);
    //
    // if (dcpFilename.empty()) {
    //     if (customIfDcpMissing) {
    //         setCameraProfileCustom();
    //         return;
    //     }
    //     throw std::runtime_error("Could not parse supplied camera profile file!");
    // }

    dng_file_stream profStream(dcpFilename.c_str());
    if (prof->ParseExtended(profStream))
        neg->AddProfile(prof);
}
void setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion, std::unique_ptr<dng_negative>&neg) {
    // We use whatever's in the source DNG and just update date and software
    dng_exif *negExif = neg->GetExif();
    negExif->fDateTime = dateTimeNow;
    negExif->fSoftware = appNameVersion;
}
void setXmpFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion, std::unique_ptr<dng_negative>&neg) {
    // We use whatever's in the source DNG and just update some base tags
    dng_xmp *negXmp = neg->GetXMP();
    negXmp->UpdateDateTime(dateTimeNow);
    negXmp->UpdateMetadataDate(dateTimeNow);
    negXmp->SetString(XMP_NS_XAP, "CreatorTool", appNameVersion);
    negXmp->Set(XMP_NS_DC, "format", "image/dng");
    negXmp->SetString(XMP_NS_PHOTOSHOP, "DateCreated", neg->GetExif()->fDateTimeOriginal.Encode_ISO_8601());
}
void buildNegative(const std::string &dcpFilename, bool customIfDcpMissing, std::unique_ptr<dng_negative>&neg ) {
    // -----------------------------------------------------------------------------------------
    // Set all metadata and properties

    // if (m_publishFunction != NULL) m_publishFunction("processing metadata");

    // m_negProcessor->setDNGPropertiesFromRaw(); // dont think it's needed
    setCameraProfileFile(dcpFilename, customIfDcpMissing, neg);

    dng_string appNameVersion("mini-lr-poc"); appNameVersion.Append(" "); appNameVersion.Append("mini-lr-0.1");
    // dng_string appNameVersion(m_appName); appNameVersion.Append(" "); appNameVersion.Append(m_appVersion.Get());
    dng_date_time_info m_dateTimeNow;
    CurrentDateTimeAndZone(m_dateTimeNow);

    setExifFromRaw(m_dateTimeNow, appNameVersion, neg);
    setXmpFromRaw(m_dateTimeNow, appNameVersion, neg);

    neg->RebuildIPTC(true);

    // m_negProcessor->backupProprietaryData();//maybe not needed because wew use dng nd not raw

    // -----------------------------------------------------------------------------------------
    // Copy raw sensor data

    // if (m_publishFunction != NULL) m_publishFunction("reading raw image data");

    // m_negProcessor->buildDNGImage(); //maybe not needed because wew use dng nd not raw
}
bool first_time = true;
void generateEditing(const std::string &path, const std::string &output, const std::string &dcpFile,std::vector<uint8>& vec, int exposure=0) {
    // std::cout << "generateEditing " << "\n";

    static dng_host h;
    // std::unique_ptr<dng_negative> negative;
    static  std::unique_ptr<dng_negative> negative(h.Make_dng_negative());
    // negative = negative->Make(h);
    // std::unique_ptr<dng_negative> negative;
    if (first_time) {
        dngProcessor(path, h, negative);
        buildNegative(dcpFile, false, negative);
        renderImage(negative, h);
        first_time = false;
    }

    std::unordered_map<std::string, real64> editingParamsMap;
    editingParamsMap["exposure"] = exposure;
    try {
        writeTiffTemplate(output, editingParamsMap, h, negative, vec);
    }
    catch (const std::exception &e) {
        std::cout << "writeTiffTemplate failed " << e.what() << std::endl;
    }
}

void read_file(const std::string &fn,std::vector<uint8>& vec, int exposure=0) {
    const std::string hardcoded_dcp_path = "/profiles/Canon_EOS_R6_Adobe_Standard.dcp";

    // std::cout << "read_file " << fn << "\n";
    try {
        generateEditing(fn, "/work/output.jpg", hardcoded_dcp_path, vec, exposure);
    }
    catch (const std::exception &e) {
        std::cout << "exception caught in  read_file" << e.what() << "\n";
        assert(false);
    }
}


// std::string read_file(const std::string &fn)
// {
//
//     // std::cout << "read_file " << fn << "\n";
//     try {
//          getInitWbPerFile(fn);
//
//     }
//     catch (const std::exception &e) {
//         std::cout << "exception caught in  read_file" << e.what() << "\n";
//         assert(false);
//     }
//     std::ifstream f(fn);
//     // std::string line;
//     // std::getline(f, line);
//     // std::cout << "line " << line << "\n";
//     // return line;
//     return "";
//     // return "finish reading files!";
// }

// std::string read_file(const std::string &fn)
// {
//
//     // std::cout << "read_file " << fn << "\n";
//     try {
//          getInitWbPerFile(fn);
//
//     }
//     catch (const std::exception &e) {
//         std::cout << "exception caught in  read_file" << e.what() << "\n";
//         assert(false);
//     }
//     std::ifstream f(fn);
//     // std::string line;
//     // std::getline(f, line);
//     // std::cout << "line " << line << "\n";
//     // return line;
//     return "";
//     // return "finish reading files!";
// }

// EMSCRIPTEN_BINDINGS(hello) {
//     function("read_file", &read_file);
// }

EMSCRIPTEN_BINDINGS(my_module) {
    function("read_file", &read_file);
    register_vector<uint8_t>("VectorUint8");

    // register_vector<uint8>("vector<uint8>");
}