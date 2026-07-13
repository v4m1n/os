module;
#include <cstdint>

export module vfs;

export namespace vfs {

enum class NodeType {
  FILE = 1,
  DIRECTORY = 2
};

struct DirectoryEntry {
  char name[256];
  uint32_t inode;
  NodeType type;
};

class VfsNode {
public:
  virtual ~VfsNode() = default;
  virtual int64_t read(uint64_t offset, uint32_t size, void *buffer) = 0;
  virtual int64_t write(uint64_t offset, uint32_t size, const void *buffer) = 0;
  virtual int readdir(uint32_t index, DirectoryEntry &entry) = 0;
  virtual VfsNode *finddir(const char *name) = 0;
  virtual uint64_t getSize() const = 0;
  virtual NodeType getType() const = 0;
};

class FileSystem {
public:
  virtual ~FileSystem() = default;
  virtual VfsNode *getRoot() = 0;
};

void init();
int mount(const char *path, FileSystem *fs);
VfsNode *getRootNode();
VfsNode *open(const char *path);

} // namespace vfs
