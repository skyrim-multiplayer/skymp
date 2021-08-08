#include "Loader.h"

#include <fmt/format.h>

// Linux
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

namespace espm {

class AllocatedBuffer : public impl::IBuffer {
public:
  AllocatedBuffer(const fs::path& path): data() {
    const auto size = fs::file_size(path);
    data.resize(size);

    std::ifstream f(path.string(), std::ios::binary);
    if (!f.read(data.data(), size)) {
      throw Loader::LoadError(fmt::format("Can't read {}", path.string()));
    }
  }

  char* GetData() override {
    return data.data();
  }

  size_t GetLength() override {
    return data.size();
  }

private:
  std::vector<char> data;
};

class MappedBuffer : public impl::IBuffer {
public:
  MappedBuffer(const fs::path& path) {
    size_ = fs::file_size(path);
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
      throw std::system_error(errno, std::generic_category(), fmt::format("Can't read {}", path.string()));
    }
    const auto mmapResult = mmap(NULL, size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (mmapResult == MAP_FAILED) {
      throw std::system_error(errno, std::generic_category(), fmt::format("Can't map {}", path.string()));
    }
    data_ = static_cast<char*>(mmapResult);
  }

  ~MappedBuffer() {
    int result = munmap(data_, size_);
    if (result) {
      abort();
    }
  }

  char* GetData() override {
    return data_;
  }

  size_t GetLength() override {
    return size_;
  }

private:
  int fd_;
  char* data_;
  size_t size_;
};

std::unique_ptr<impl::IBuffer> Loader::MakeBuffer(const fs::path& filePath) const {
  // return std::unique_ptr<impl::IBuffer>{new AllocatedBuffer(filePath)};
  return std::unique_ptr<impl::IBuffer>{new MappedBuffer(filePath)};
}

}  // namespace espm
