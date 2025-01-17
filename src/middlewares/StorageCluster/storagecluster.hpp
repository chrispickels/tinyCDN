#pragma once

#include <memory>
#include <mutex>
#include <experimental/filesystem>

#include "../Volume/volume.hpp"
#include "../Volume/marshaller.hpp"
#include "../../hashing.hpp"

#include "marshaller.hpp"
#include "request.hpp"
#include "response.hpp"
#include "services.hpp"

namespace fs = std::experimental::filesystem;

namespace TinyCDN::Middleware::StorageCluster {

using namespace TinyCDN::Utility::Hashing;
using namespace TinyCDN::Middleware::Volume;

struct MasterRequest;
struct MasterResponse;
class StorageClusterRequest;
class StorageClusterResponse;

struct StorageClusterParams {
  UUID4 id;
  std::string name;
  fs::path location;
//  VirtualVolumeParams virtualVolume;
};

/*!
 * \brief A single "core" allocated with available storage.
 * Hosts a virtual volume and receives commands from Master node to store/retrieve files to/from filebuckets, or replicate filebuckets, etc.
Gets a authentication key from the Master node that gives permission to store/retrieve a file.
 */
class StorageClusterNode {
public:
  UUID4 id;
  std::string name;
  fs::path location;

  std::unique_ptr<VirtualVolume> virtualVolume;

  std::unique_ptr<StorageFileHostingService> getHostingService();
  std::unique_ptr<StorageFileUploadingService> getUploadingService();

  // std::map<SessionId, StorageFileHostingSession>

  void configure(StorageClusterParams params);

  StorageClusterNode() {};

private:
  // TODO: object pool of services
  /*
    This is where we can create the most threads.
    Each service will exist in its own thread.
    The service shares access to the virtual volume's volumemanager and access to the lookup of storagevolumes
  */
  std::unique_ptr<StorageFileUploadingService> uploadingService;
  std::unique_ptr<StorageFileHostingService> hostingService;

  // TODO pool of services... see above
  std::mutex uploadServiceMutex;
  std::mutex hostingServiceMutex;
  std::mutex virtualVolumeMutex;

  VirtualVolumeJsonMarshaller volumeMarshaller;

  // TODO: networking
  // masterSocket
};

struct StorageClusterNodeSingleton {
  static StorageClusterNode* getInstance();
  static StorageClusterNode instance;
};

class StorageClusterSession {
public:
  // Only shared because we would like to copy StorageClusterSession. All locks will be unique_locks
  std::shared_mutex storageClusterNodeMutex;
  fs::path configFileLocation;

  StorageClusterParams loadConfig(fs::path location);
  void spawn();

  void startSession(fs::path configFileLocation) {
    if (started) return;

    // TODO: Networking - Wait forever for initialization packet from Master
    // wait ()
    std::ifstream configFile(this->configFileLocation);

    // TODO Notify Master of failure to load
    if (!configFile.is_open() || configFile.bad()) return;

    StorageClusterNodeJsonMarshaller marshaller;

    std::string config((std::istreambuf_iterator<char>(configFile)),
		       std::istreambuf_iterator<char>());
    configFile.close();
  }

  //! Returns a raw pointer to the storageCluster node along with a lock that should be unlocked once the storageCluster node is no longer needed
  inline std::tuple<std::unique_lock<std::shared_mutex>, StorageClusterNode*> getStorageClusterNode() {
    // acquire storageCluster unique_lock
    // timeout for mutex?

    //    storageClusterSession.storageClusterNodeMutex;
    std::unique_lock<std::shared_mutex> lock(storageClusterNodeMutex);

    return { std::move(lock), singleton.getInstance() };
  }

  // HTTP frontend: implement the following
  // parseStorageClusterMasterRequest(std::string message); -> StorageClusterRequest
  // parseStorageClusterClientRequest(std::string message); -> StorageClusterRequest

  // TODO: Use Command pattern
  StorageClusterResponse receiveMasterCommand(StorageClusterRequest request);
  void sendMasterCommand(MasterRequest request);
  // NOTE: The client does not need TCP wrappers, client communication going from HTTP -> CFFI
  // StorageClusterResponse receiveClientCommand(StorageClusterRequest request);
  // StorageClusterResponse sendClientCommand(StorageClusterRequest request);


private:
  bool started;
  StorageClusterNodeSingleton singleton;
};

}
