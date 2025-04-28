
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
dng_file_stream* openFileStream(const std::string &outFilename) {

    try {
        std::cout << "before  fopen " << outFilename << "\n";

        FILE* f = fopen(outFilename.c_str(), "w");

        std::cout << "after fopen " << outFilename << "\n";
        bool x = f == nullptr;
        std::cout << "f is null? " << x << "\n";
        return new dng_file_stream(f );
        // return new dng_file_stream(outFilename.c_str(), true);
    }
    catch (dng_exception& e) {
        // std::stringstream error; error << "Error opening output file! (" << e.ErrorCode() << ": " << (e.ErrorCode()) << ")";
        throw std::runtime_error("error.str()");
        // throw std::runtime_error(error.str());
    }
}

void writeJpeg(const std::string& outFilename, real64 exposure, real64 shadows, const dng_xy_coord& xyCoordinate,  dng_host& host, std::unique_ptr<dng_negative>&neg) {
    // -----------------------------------------------------------------------------------------
    // Render JPEG

    // FIXME: we should render and integrate a thumbnail too
    // if (m_publishFunction != NULL) m_publishFunction("rendering JPEG");

    dng_render negRender(host, *neg);
    negRender.SetWhiteXY(xyCoordinate);

    if (exposure) {
        negRender.SetExposure(exposure);
    }
    if (shadows) {
        negRender.SetShadows(shadows);
    }    AutoPtr<dng_image> negImage(negRender.Render());
    dng_string appNameVersion("mini-lr-poc"); appNameVersion.Append(" "); appNameVersion.Append("mini-lr-0.1");

    AutoPtr<dng_jpeg_preview> jpeg(new dng_jpeg_preview());
    jpeg->fInfo.fApplicationName.Set_ASCII(appNameVersion.Get());
    jpeg->fInfo.fApplicationVersion.Set_ASCII(appNameVersion.Get());
    dng_date_time_info m_dateTimeNow;
    CurrentDateTimeAndZone(m_dateTimeNow);
    jpeg->fInfo.fDateTime = m_dateTimeNow.Encode_ISO_8601();
    jpeg->fInfo.fColorSpace = previewColorSpace_sRGB;
    dng_image_writer jpegWriter; jpegWriter.EncodeJPEGPreview(host, *negImage.Get(), *jpeg.Get(), 8);
    // -----------------------------------------------------------------------------------------
    // Write JPEG-image to file
    // if (m_publishFunction != NULL) m_publishFunction("writing JPEG file");
    // AutoPtr<dng_file_stream> targetFile(openFileStream(outFilename));
    // AutoPtr<dng_memory_stream> targetFile(host.Allocator ());
    dng_memory_stream targetFile (host.Allocator ());

    const uint8 soiTag[]         = {0xff, 0xd8};
    const uint8 app1Tag[]        = {0xff, 0xe1};
    const char* app1ExifHeader   = "Exif\0";
    const int exifHeaderLength   = 6;
    const uint8 tiffHeader[]     = {0x49, 0x49, 0x2a, 0x00, 0x08, 0x00, 0x00, 0x00};
    const char* app1XmpHeader    = "http://ns.adobe.com/xap/1.0/";
    const int xmpHeaderLength    = 29;
    const char* app1ExtXmpHeader = "http://ns.adobe.com/xmp/extension/";
    const int extXmpHeaderLength = 35;
    const int jfifHeaderLength   = 20;
    // hack: we're overloading the class just to get access to protected members (DNG-SDK doesn't exposure full Put()-function on these)
    class ExifIfds : public exif_tag_set {
    public:
        dng_tiff_directory* getExifIfd() {return &fExifIFD;}
        dng_tiff_directory* getGpsIfd() {return &fGPSIFD;}
        ExifIfds(dng_tiff_directory &directory, const dng_exif &exif, dng_metadata* md) :
            exif_tag_set(directory, exif, md->IsMakerNoteSafe(), md->MakerNoteData(), md->MakerNoteLength(), false) {}
    };
    try {
        // -----------------------------------------------------------------------------------------
        // Build IFD0, ExifIFD, GPSIFD
        dng_metadata* metadata = &neg->Metadata();

        metadata->GetXMP()->Set(XMP_NS_DC, "format", "image/jpeg");

        dng_tiff_directory mainIfd;
        tag_uint16 tagOrientation(tcOrientation, metadata->BaseOrientation().GetTIFF());
        mainIfd.Add(&tagOrientation);
        // this is non-standard I believe but let's leave it anyway
        tag_iptc tagIPTC(metadata->IPTCData(), metadata->IPTCLength());
        if (tagIPTC.Count()) mainIfd.Add(&tagIPTC);
        // this creates exif and gps Ifd and also adds the following to mainIfd:
        // datetime, imagedescription, make, model, software, artist, copyright, exifIfd, gpsIfd
        ExifIfds exifSet(mainIfd, *metadata->GetExif(), metadata);
        exifSet.Locate(sizeof(tiffHeader) + mainIfd.Size());
        // we're ignoring YCbCrPositioning, XResolution, YResolution, ResolutionUnit
        // YCbCrCoefficients, ReferenceBlackWhite
        // -----------------------------------------------------------------------------------------
        // Build IFD0, ExifIFD, GPSIFD
        // Write SOI-tag
        targetFile.Put(soiTag, sizeof(soiTag));
        // Write APP1-Exif section: Header...
        targetFile.Put(app1Tag, sizeof(app1Tag));
        targetFile.SetBigEndian(true);
        targetFile.Put_uint16(sizeof(uint16) + exifHeaderLength + sizeof(tiffHeader) + mainIfd.Size() + exifSet.Size());
        targetFile.Put(app1ExifHeader, exifHeaderLength);
        // ...and TIFF structure
        targetFile.SetLittleEndian(true);
        targetFile.Put(tiffHeader, sizeof(tiffHeader));
        mainIfd.Put(targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader));
        exifSet.getExifIfd()->Put(targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader) + mainIfd.Size());
        exifSet.getGpsIfd()->Put(targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader) + mainIfd.Size() + exifSet.getExifIfd()->Size());
        // Write APP1-XMP if required
        if (metadata->GetXMP()) {
            AutoPtr<dng_memory_block> stdBlock, extBlock;
            dng_string extDigest;
            metadata->GetXMP()->PackageForJPEG(stdBlock, extBlock, extDigest);
            targetFile.Put(app1Tag, sizeof(app1Tag));
            targetFile.SetBigEndian(true);
            targetFile.Put_uint16(sizeof(uint16) + xmpHeaderLength + stdBlock->LogicalSize());
            targetFile.Put(app1XmpHeader, xmpHeaderLength);
            targetFile.Put(stdBlock->Buffer(), stdBlock->LogicalSize());
            if (extBlock.Get()) {
                // we only support one extended block, if XMP is >128k the file will probably be corrupted
                targetFile.Put(app1Tag, sizeof(app1Tag));
                targetFile.SetBigEndian(true);
                targetFile.Put_uint16(sizeof(uint16) + extXmpHeaderLength + extDigest.Length() + sizeof(uint32) + sizeof(uint32) + extBlock->LogicalSize());
                targetFile.Put(app1ExtXmpHeader, extXmpHeaderLength);
                targetFile.Put(extDigest.Get(), extDigest.Length());
                targetFile.Put_uint32(extBlock->LogicalSize());
                targetFile.Put_uint32(stdBlock->LogicalSize());
                targetFile.Put(extBlock->Buffer(), extBlock->LogicalSize());
            }
        }
        // write remaining JPEG structure/data from libjpeg minus the JFIF-header
        targetFile.Put((uint8*) jpeg->CompressedData().Buffer() + jfifHeaderLength, jpeg->CompressedData().LogicalSize() - jfifHeaderLength);
        targetFile.Flush();
    }
    catch (dng_exception& e) {
        std::cout << "error in jpeg writer" << std::endl;
        // std::stringstream error; error << "Error while writing JPEG-file! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        // throw std::runtime_error(error.str());
    }
}


std::vector<char> writeTiffTemplate(const std::string& outFilename, std::unordered_map<std::string, real64>& editingParamsMap, dng_host& host, std::unique_ptr<dng_negative>&neg) {
    // -----------------------------------------------------------------------------------------
    // Render TIFF

    // if (m_publishFunction != NULL) m_publishFunction("rendering TIFF");
    std::cout << "writeTiffTemplate strat " << std::endl;

    dng_render negRender(host, *neg);
    // dng_render negRender(*m_host, *m_negProcessor->getNegative());


    negRender.SetFinalPixelType(ttShort);

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
    std::cout << "writeTiffTemplate before calling Render() " << std::endl;

    AutoPtr<dng_image> negImage(negRender.Render());
    std::cout << "writeTiffTemplate after calling Render() " << std::endl;

    // -----------------------------------------------------------------------------------------
    // Write Tiff-image to file

    // AutoPtr<dng_file_stream> targetFile(openFileStream(outFilename));
    // AutoPtr<dng_memory_stream> target(host.Allocator());
    dng_memory_stream stream (host.Allocator ());

    std::cout << "writeTiffTemplate targetFile " << outFilename <<  std::endl;

    // dng_pixel_buffer pixel_buf;
    // dng_simple_image& y = dynamic_cast<dng_simple_image&>(*negImage);
    //
    // // dng_simple_image *i = &dstImage;
    // y.GetPixelBuffer(pixel_buf);
    // WriteSeparatePlanes3(pixel_buf, y.Bounds().W(), y.Bounds().H());
   // if (m_publishFunction != NULL) m_publishFunction("writing TIFF file");

    try {
        // dng_image_writer tiffWriter;
        // auto metadata = &neg->Metadata();
        std::cout << "before  AutoPtr<dng_jpeg_preview> jpeg setup" << std::endl;

        AutoPtr<dng_jpeg_preview> jpeg(new dng_jpeg_preview());
        dng_string appNameVersion("mini-lr-poc"); appNameVersion.Append(" "); appNameVersion.Append("mini-lr-0.1");
        // dng_string appNameVersion(m_appName); appNameVersion.Append(" "); appNameVersion.Append(m_appVersion.Get());
        dng_date_time_info m_dateTimeNow;
        CurrentDateTimeAndZone(m_dateTimeNow);

        jpeg->fInfo.fApplicationName.Set_ASCII(appNameVersion.Get());
        jpeg->fInfo.fApplicationVersion.Set_ASCII(appNameVersion.Get());
        jpeg->fInfo.fDateTime = m_dateTimeNow.Encode_ISO_8601();
        jpeg->fInfo.fColorSpace = previewColorSpace_sRGB;

        std::cout << "before  EncodeJPEGPreview" << std::endl;

        dng_image_writer jpegWriter; jpegWriter.EncodeJPEGPreview(host, *negImage.Get(), *jpeg.Get(), 8);
        std::cout << "after  EncodeJPEGPreview" << std::endl;

        auto& block = jpeg->CompressedData();
        auto buf = jpeg->CompressedData().Buffer_uint8();
        size_t size = block.LogicalSize();
        std::cout << "before assigning vec  EncodeJPEGPreview" << std::endl;

        std::vector<char> vec(buf, buf + size);
        return vec;
        // tiffWriter.WriteTIFF(host, stream, *negImage.Get(), piRGB, ccUncompressed,
        //                      metadata, &dng_space_sRGB::Get());
        // std::cout << "after   tiffWriter.WriteTIFF" << std::endl;
        //
        // // fs::path(outFilename).replace_extension(".jpg");
        // writeJpeg("out.jpg", negRender.Exposure(), negRender.Shadows(), negRender.WhiteXY(), host, neg);
    }
    catch (dng_exception& e) {
       std::cout << "Error while writing TIFF-file! " << e.ErrorCode() << std::endl;
    }
}
void renderImage(std::unique_ptr<dng_negative>&neg, dng_host &host) {
    // -----------------------------------------------------------------------------------------
    // Render image

    try {
        // if (m_publishFunction != NULL) m_publishFunction("building preview - linearising");

        neg->BuildStage2Image(host);   // Compute linearized and range-mapped image

        // if (m_publishFunction != NULL) m_publishFunction("building preview - demosaicing");

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

std::vector<char> generateEditing(const std::string &path, const std::string &output, const std::string &dcpFile) {
    std::cout << "generateEditing " << "\n";

    dng_host h;
    std::unique_ptr<dng_negative> negative(h.Make_dng_negative());

    try {
        dngProcessor(path, h, negative);
    }
    catch (const std::exception &e)
    {
        std::cout << "dngProcessor failed" << e.what() << std::endl;
    }
    try {
        buildNegative(dcpFile, false, negative);
    }
    catch (const std::exception &e)
    {
        std::cout << "buildNegative failed"<< e.what() << std::endl;
    }
    try {
        renderImage(negative, h);

    }
    catch (const std::exception &e)
    {
        std::cout << "renderImage failed"<< e.what() << std::endl;
    }
    std::unordered_map<std::string, real64> editingParamsMap;
    editingParamsMap["exposure"] = 0;
    try {
        std::vector<char> vec  = writeTiffTemplate(output, editingParamsMap, h, negative);
        std::cout << "vec size " << vec.size() << std::endl;
        return vec;

    }
    catch (const std::exception &e) {
        std::cout << "writeTiffTemplate failed " << e.what() << std::endl;
    }
}
// std::string get_dcp_file (const std::string &dcp_file_path )
// {
//
//     std::cout << "get_dcp_file " << dcp_file_path << "\n";
//     try {
//         // dng_file_stream stream(dcp_file_path.c_str(), false, dng_stream::kBigBufferSize);
//         return dcp_file_path;
//     }
//     catch (const std::exception &e) {
//         std::cout << "exception caught in  read_file" << e.what() << "\n";
//         assert(false);
//     }
//     // std::string line;
//     // std::getline(f, line);
//     // std::cout << "line " << line << "\n";
//     // return line;
//     return "finish reading files!";
// }

std::vector<char> read_file(const std::string &fn)
{
    const std::string hardcoded_dcp_path = "/profiles/Canon_EOS_R6_Adobe_Standard.dcp";

    std::cout << "read_file " << fn << "\n";
    try {
        auto vec = generateEditing(fn, "/work/output.jpg", hardcoded_dcp_path);
        return vec;
        // getInitWbPerFile(fn);

    }
    catch (const std::exception &e) {
        std::cout << "exception caught in  read_file" << e.what() << "\n";
        assert(false);
    }
    std::ifstream f(fn);
    // std::string line;
    // std::getline(f, line);
    // std::cout << "line " << line << "\n";
    // return line;
    return {};
    // return "finish reading files!";
}

// std::string get_dcp_file (const std::string &dcp_file_path )
// {
//
//     std::cout << "get_dcp_file " << dcp_file_path << "\n";
//     try {
//         // dng_file_stream stream(dcp_file_path.c_str(), false, dng_stream::kBigBufferSize);
//         return dcp_file_path;
//     }
//     catch (const std::exception &e) {
//         std::cout << "exception caught in  read_file" << e.what() << "\n";
//         assert(false);
//     }
//     // std::string line;
//     // std::getline(f, line);
//     // std::cout << "line " << line << "\n";
//     // return line;
//     return "finish reading files!";
// }
//
// std::string read_file(const std::string &fn)
// {
//
//     std::cout << "read_file " << fn << "\n";
//     try {
         // auto vec = generateEditing(fn, "/work/output.jpg", ".");
         // getInitWbPerFile(fn);
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
//     return "finish reading files!";
// }

// EMSCRIPTEN_BINDINGS(hello) {
//     function("read_file", &read_file);
// }

EMSCRIPTEN_BINDINGS(my_module) {
    // function("get_dcp_file", &get_dcp_file);
    function("read_file", &read_file);
    register_vector<char>("vector<char>");
}