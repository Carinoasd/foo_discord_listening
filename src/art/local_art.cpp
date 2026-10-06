// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/local_art.h"

#include <wincodec.h>
#include <shlwapi.h>

#include <atlbase.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

namespace fdl::art {
namespace {

album_art_data::ptr LoadFrontCover(const metadb_handle_ptr& track, abort_callback& abort) {
    auto api = album_art_manager_v3::get();
    pfc::list_single_ref_t<metadb_handle_ptr> items(track);
    pfc::list_single_ref_t<GUID> ids(album_art_ids::cover_front);
    auto extractor = api->open_v3(items, ids, nullptr, abort);
    try {
        return extractor->query(album_art_ids::cover_front, abort);
    } catch (const exception_album_art_not_found&) {
        return {};
    }
}

void Check(HRESULT hr, const char* what) {
    if (FAILED(hr)) {
        throw std::runtime_error(std::string(what) + " failed (0x" + std::format("{:08X}", static_cast<uint32_t>(hr)) + ")");
    }
}

std::filesystem::path UniqueTempFile() {
    wchar_t dir[MAX_PATH + 1]{};
    GetTempPathW(MAX_PATH, dir);
    GUID guid{};
    CoCreateGuid(&guid);
    wchar_t name[64]{};
    swprintf_s(name, L"fdl-art-%08lX%04X%04X.jpg", guid.Data1, guid.Data2, guid.Data3);
    return std::filesystem::path(dir) / name;
}

} // namespace

std::optional<std::filesystem::path> ExportCoverArt(const metadb_handle_ptr& track, unsigned max_size, abort_callback& abort) {
    const auto data = LoadFrontCover(track, abort);
    if (!data.is_valid() || data->get_size() == 0) {
        return std::nullopt;
    }

    CComPtr<IWICImagingFactory> factory;
    Check(factory.CoCreateInstance(CLSID_WICImagingFactory), "WIC factory");

    CComPtr<IStream> input;
    input.Attach(SHCreateMemStream(static_cast<const BYTE*>(data->get_ptr()), static_cast<UINT>(data->get_size())));
    if (!input) {
        throw std::runtime_error("SHCreateMemStream failed");
    }
    CComPtr<IWICBitmapDecoder> decoder;
    Check(factory->CreateDecoderFromStream(input, nullptr, WICDecodeMetadataCacheOnDemand, &decoder), "decode cover art");
    CComPtr<IWICBitmapFrameDecode> frame;
    Check(decoder->GetFrame(0, &frame), "decode cover art frame");

    UINT width = 0;
    UINT height = 0;
    Check(frame->GetSize(&width, &height), "read cover art size");
    if (width == 0 || height == 0) {
        return std::nullopt;
    }
    const double scale = (std::min)(1.0, static_cast<double>(max_size) / (std::max)(width, height));
    const UINT out_w = (std::max<UINT>)(1, static_cast<UINT>(width * scale));
    const UINT out_h = (std::max<UINT>)(1, static_cast<UINT>(height * scale));

    CComPtr<IWICBitmapScaler> scaler;
    Check(factory->CreateBitmapScaler(&scaler), "create scaler");
    Check(scaler->Initialize(frame, out_w, out_h, WICBitmapInterpolationModeHighQualityCubic), "scale cover art");

    // JPEG 編碼器只接受 24bpp BGR，先轉格式；PNG 的 alpha 通道會被直接捨棄。
    CComPtr<IWICFormatConverter> converter;
    Check(factory->CreateFormatConverter(&converter), "create converter");
    Check(converter->Initialize(scaler, GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom),
          "convert cover art");

    const auto path = UniqueTempFile();
    CComPtr<IWICStream> output;
    Check(factory->CreateStream(&output), "create output stream");
    Check(output->InitializeFromFilename(path.c_str(), GENERIC_WRITE), "open temp file");

    try {
        CComPtr<IWICBitmapEncoder> encoder;
        Check(factory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &encoder), "create JPEG encoder");
        Check(encoder->Initialize(output, WICBitmapEncoderNoCache), "init JPEG encoder");
        CComPtr<IWICBitmapFrameEncode> out_frame;
        CComPtr<IPropertyBag2> props;
        Check(encoder->CreateNewFrame(&out_frame, &props), "create JPEG frame");
        PROPBAG2 option{};
        option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        VARIANT quality{};
        quality.vt = VT_R4;
        quality.fltVal = 0.9f;
        props->Write(1, &option, &quality);
        Check(out_frame->Initialize(props), "init JPEG frame");
        Check(out_frame->SetSize(out_w, out_h), "set JPEG size");
        WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
        Check(out_frame->SetPixelFormat(&format), "set JPEG format");
        Check(out_frame->WriteSource(converter, nullptr), "write JPEG");
        Check(out_frame->Commit(), "commit JPEG frame");
        Check(encoder->Commit(), "commit JPEG");
    } catch (...) {
        output.Release();
        std::error_code ec;
        std::filesystem::remove(path, ec);
        throw;
    }
    return path;
}

} // namespace fdl::art
