module;
#include <cstddef>

module vfs;
import string;
import kmm;
import sync;

namespace vfs {

static FileSystem *root_fs = nullptr;
static VfsNode *root_node = nullptr;
static mutex vfs_mutex;

void init() {
  // Initialization of VFS
}

int mount(const char *path, FileSystem *fs) {
  lock_guard<mutex> guard(vfs_mutex);
  if (strcmp(path, "/") == 0) {
    root_fs = fs;
    root_node = fs->getRoot();
    return 0;
  }
  return -1;
}

VfsNode *getRootNode() {
  lock_guard<mutex> guard(vfs_mutex);
  return root_node;
}

VfsNode *open(const char *path) {
  lock_guard<mutex> guard(vfs_mutex);
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

static VfsNode *resolve_parent(const char *path, char *out_name) {
  if (!root_node) return nullptr;
  if (path[0] != '/') return nullptr;

  VfsNode *curr = root_node;
  const char *ptr = path;

  while (*ptr == '/') ptr++;
  if (*ptr == '\0') return nullptr;

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

    while (*ptr == '/') ptr++;

    if (*ptr == '\0') {
      memcpy(out_name, token, len + 1);
      return curr;
    }

    if (curr->getType() != NodeType::DIRECTORY) {
      if (curr != root_node) delete curr;
      return nullptr;
    }
    VfsNode *next = curr->finddir(token);
    if (curr != root_node) delete curr;
    curr = next;
    if (!curr) return nullptr;
  }
  return nullptr;
}

VfsNode *create(const char *path, NodeType type) {
  lock_guard<mutex> guard(vfs_mutex);
  char name[256];
  VfsNode *parent = resolve_parent(path, name);
  if (!parent) return nullptr;
  VfsNode *node = parent->create(name, type);
  if (parent != root_node) delete parent;
  return node;
}

int unlink(const char *path) {
  lock_guard<mutex> guard(vfs_mutex);
  char name[256];
  VfsNode *parent = resolve_parent(path, name);
  if (!parent) return -1;
  int res = parent->unlink(name);
  if (parent != root_node) delete parent;
  return res;
}

} // namespace vfs
