#include "vfs.h"
#include "string.h"
#include "kmm.h"

namespace vfs {

static FileSystem *root_fs = nullptr;
static VfsNode *root_node = nullptr;

void init() {
  // Initialization of VFS
}

int mount(const char *path, FileSystem *fs) {
  if (strcmp(path, "/") == 0) {
    root_fs = fs;
    root_node = fs->getRoot();
    return 0;
  }
  return -1;
}

VfsNode *getRootNode() {
  return root_node;
}

VfsNode *open(const char *path) {
  if (!root_node) return nullptr;
  if (path[0] != '/') return nullptr;
  if (path[1] == '\0') return root_node;

  VfsNode *curr = root_node;
  const char *ptr = path;

  while (*ptr == '/') {
    ptr++;
  }

  while (*ptr != '\0') {
    char token[256];
    size_t len = 0;
    while (*ptr != '\0' && *ptr != '/') {
      if (len < sizeof(token) - 1) {
        token[len++] = *ptr;
      }
      ptr++;
    }
    token[len] = '\0';

    if (len > 0) {
      if (curr->getType() != NodeType::DIRECTORY) {
        if (curr != root_node) delete curr;
        return nullptr;
      }
      VfsNode *next = curr->finddir(token);
      if (curr != root_node) {
        delete curr;
      }
      curr = next;
      if (!curr) return nullptr;
    }

    while (*ptr == '/') {
      ptr++;
    }
  }

  return curr;
}

} // namespace vfs
