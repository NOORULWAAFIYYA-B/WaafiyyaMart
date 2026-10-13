#include "ProductImageStorage.h"

#include <drogon/drogon.h>
#include <openssl/rand.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
std::filesystem::path getStorageDirectory()
{
    const auto* configured = std::getenv("PRODUCT_IMAGE_DIR");
    if (configured && *configured)
    {
        return std::filesystem::path(configured);
    }

    if (std::getenv("RENDER"))
    {
        throw ProductImageStorage::StorageError(
            "Persistent product image storage is not configured");
    }

    return std::filesystem::path("uploads");
}

std::string randomFileId()
{
    std::array<unsigned char, 16> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1)
    {
        throw std::runtime_error("Unable to generate an image filename");
    }

    std::ostringstream id;
    id << std::hex << std::setfill('0');
    for (const auto byte : bytes)
    {
        id << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return id.str();
}

bool isSafeFilename(const std::string& filename)
{
    if (filename.size() < 36 || filename.size() > 40)
    {
        return false;
    }

    const auto dot = filename.find('.');
    if (dot != 32 || dot == std::string::npos)
    {
        return false;
    }
    for (std::size_t i = 0; i < dot; ++i)
    {
        const auto c = filename[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
        {
            return false;
        }
    }
    const auto extension = filename.substr(dot);
    return extension == ".jpg" || extension == ".png" ||
           extension == ".webp";
}
}

ProductImageStorage::ImageValidationResult ProductImageStorage::validate(
    std::string_view content,
    std::string& extension)
{
    if (content.empty())
    {
        return ImageValidationResult::Missing;
    }
    if (content.size() > kMaxImageSize)
    {
        return ImageValidationResult::TooLarge;
    }

    const auto byte = [&content](std::size_t index) {
        return static_cast<unsigned char>(content[index]);
    };

    static constexpr unsigned char pngSignature[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    if (content.size() >= sizeof(pngSignature) &&
        content.compare(0, sizeof(pngSignature),
                        reinterpret_cast<const char*>(pngSignature),
                        sizeof(pngSignature)) == 0)
    {
        extension = ".png";
        return ImageValidationResult::Valid;
    }

    if (content.size() >= 12 &&
        content.compare(0, 4, "RIFF", 4) == 0 &&
        content.compare(8, 4, "WEBP", 4) == 0)
    {
        extension = ".webp";
        return ImageValidationResult::Valid;
    }

    if (content.size() >= 3 && byte(0) == 0xff && byte(1) == 0xd8 &&
        byte(2) == 0xff)
    {
        extension = ".jpg";
        return ImageValidationResult::Valid;
    }

    return ImageValidationResult::Unsupported;
}

ProductImageStorage::SavedImage ProductImageStorage::save(
    std::string_view content,
    const std::string& extension)
{
    const auto directory = getStorageDirectory();
    try
    {
        std::filesystem::create_directories(directory);
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        throw StorageError(error.what());
    }

    const auto filename = randomFileId() + extension;
    const auto path = directory / filename;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        throw StorageError("Unable to open image storage");
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    output.close();
    if (!output)
    {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        throw StorageError("Unable to write product image");
    }

    return SavedImage{"/uploads/" + filename, path.string()};
}

void ProductImageStorage::remove(const std::string& path) noexcept
{
    std::error_code error;
    if (!std::filesystem::remove(path, error) && error)
    {
        LOG_WARN << "Unable to remove unused uploaded image: "
                 << error.message();
    }
}

bool ProductImageStorage::resolveUrl(const std::string& url,
                                     std::string& path,
                                     std::string& mimeType)
{
    constexpr std::string_view prefix = "/uploads/";
    if (url.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    const auto filename = url.substr(prefix.size());
    if (!isSafeFilename(filename))
    {
        return false;
    }

    const auto extension = filename.substr(filename.find('.'));
    if (extension == ".jpg")
    {
        mimeType = "image/jpeg";
    }
    else if (extension == ".png")
    {
        mimeType = "image/png";
    }
    else
    {
        mimeType = "image/webp";
    }

    path = (getStorageDirectory() / filename).string();
    return true;
}
