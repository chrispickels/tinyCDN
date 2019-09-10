#pragma once

#include <vector>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <optional>

#include "../../utility.hpp"
#include "../../hashing.hpp"

using namespace TinyCDN::Utility::Exceptions;
using namespace TinyCDN::Utility::Hashing;

namespace TinyCDN::Middleware::Volume {

using TinyCDN::Utility::Size;

using FileBucketId = Id<64>;
using VolumeId = Id<32>;

/*!
 * \brief Abstract Static size blob
*/
class Volume {
public:
  // TODO remove this? StorageVolumeManager needs this, not Volume itself
  const VolumeId id;
  //! The maximum allowed size
  Size size;

  // virtual void resize(t size) = 0;
  virtual void destroy() = 0;

  // bool hasBucketId(FileBucketId id);

  inline Volume(VolumeId id, Size size) : id(id), size(size) {};
};

/*! Volume capable of storing files.
 * FileBuckets are assigned volumes based on location, config, etc.
 A StorageVolume will be assigned many different buckets from a VirtualVolume, but if a bucket does not populate, it can "spill over" into another StorageVolume if necessary.
 At some point, every now and then the volume can reallocate space by reclaiming lost volume space. (?)
 A StorageVolume is not be tied to the storage requirements of its buckets.
 StorageVolumes are replicated at a lower level than buckets. Buckets with default replication settings will be replicated as part of a volume manager's replication strategy.
*/
template <typename storageType>
class StorageVolume : Volume {
public:
  inline void resize() {
    throw new NotImplementedException();
  }

  inline void destroy() {
    throw new NotImplementedException();
  }

  //! A file storage driver that provides methods to retrieve, modify, and delete files
  std::unique_ptr<storageType> storage;

  inline StorageVolume(Size size, fs::path location, bool preallocated)
    : Volume(id, size) {
    storage = std::make_unique<storageType>(size, location, preallocated);

  }
  // TODO delete default, volume constructors
};

//! All StorageVolume types
using AnyStorageVolume = std::variant<StorageVolume<FileStorage::FilesystemStorage>>;
// Could be any StorageVolume instance, or a non-existent value
// Could use std::optional, but wrapping variant would make std::visit less usable
using MaybeAnyStorageVolume = std::variant<std::monostate, StorageVolume<FileStorage::FilesystemStorage>>;

//class BackupVolume : Volume;

/*!
 * \brief Responsible for allocating, replicating, and removing volumes as necessary
 */
class StorageVolumeManager {
public:
  uintmax_t getSize();
  //! Checks if size is going DOWN, if so, will replicate elsewhere?
  void setSize(uintmax_t size);

  std::unique_ptr<AnyStorageVolume> getStorageVolume(VolumeId id);
  template <typename T>
  std::unique_ptr<StorageVolume<T>> createStorageVolume(fs::path location);
  // replicateVolume
  void removeVolume(VolumeId id);

  inline StorageVolumeManager(uintmax_t size) : size(size) {}

private:
  uintmax_t size;

  std::unordered_map<VolumeId, std::unique_ptr<MaybeAnyStorageVolume>, IdHasher> volumes;
  std::unordered_map<VolumeId, std::shared_mutex, IdHasher> volumeMutexes;
};

/*!
 * \brief Volume that can be of a very large indeterminate size and manages many volumes within it.
 * The simple difference between a VirtualVolume and a StorageVolume is that a virtual volume is an alias to a single Volume that manages many volumes internally to efficently store large amounts of files.
 A FileBucket references a single virtual volume instead of the set of StorageVolumes that store its files.
*/
class VirtualVolume : Volume {
public:
  fs::path location;
  std::optional<std::vector<VolumeId>> getVolumeIds();
  //! Also modifies storage volume manager's size
  void setSize(uintmax_t size);
  StorageVolumeManager storageVolumeManager;

private:
  uintmax_t size;
  std::unordered_map<FileBucketId, std::vector<VolumeId>, IdHasher> fbVolDb;

};
}
