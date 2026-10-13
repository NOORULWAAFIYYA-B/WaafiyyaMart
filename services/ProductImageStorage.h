#pragma once

#include <string>
#include <string_view>
#include <stdexcept>

namespace ProductImageStorage
{
constexpr std::size_t kMaxImageSize = 5 * 1024 * 1024;

class StorageError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

enum class ImageValidationResult
{
    Valid,
    Missing,
    TooLarge,
    Unsupported
};

struct SavedImage
{
    std::string url;
    std::string path;
};

ImageValidationResult validate(std::string_view content,
                               std::string& extension);
SavedImage save(std::string_view content, const std::string& extension);
void remove(const std::string& path) noexcept;
bool resolveUrl(const std::string& url, std::string& path, std::string& mimeType);
}
