#define CATCH_CONFIG_MAIN

#include <tuple>
#include <functional>

#include "include/catch.hpp"

#include "src/middlewares/file.hpp"
#include "src/middlewares/Master/master.hpp"
#include "src/middlewares/StorageCluster/storagecluster.hpp"

namespace file = TinyCDN::Middleware::File;
namespace storage = TinyCDN::Middleware::FileStorage;

using namespace std::placeholders;
using namespace TinyCDN;
using namespace TinyCDN::Utility;
using namespace TinyCDN::Middleware;
using namespace TinyCDN::Middleware::Master;
using namespace TinyCDN::Middleware::Volume;
using namespace TinyCDN::Middleware::StorageCluster;

namespace fs = std::experimental::filesystem;

using fbInputArgs = std::tuple<Size, std::vector<std::string>>;


auto static fbArgs = std::vector<fbInputArgs>{
  std::make_tuple(Size{1_mB}, std::vector<std::string>{std::string("test")}),
  std::make_tuple(Size{2_mB}, std::vector<std::string>{std::string("test")})
};

SCENARIO("A new CDN is created") {

  GIVEN("a new Master Session and StorageCluster") {
    MasterSession masterSession;
    auto [masterLock, master] = masterSession.getMasterNode();
    master->existing = false;

    StorageClusterSession storageClusterSession;
    auto [storageClusterLock, storageCluster] = storageClusterSession.getStorageClusterNode();
    // storageCluster->existing = false;


    // WHEN("The storage cluster is spawned without a valid configuration") {
    // TODO when a storage cluster is created...
    WHEN("The storage cluster is spawned with a valid configuration") {
      storageCluster->configure(storageClusterSession.loadConfig(fs::path{"storage.json"}));
      storageClusterSession.spawn();

      // TODO test for TCP server on port 6498
      // TODO networking tests
      // REQUIRE(storageClusterSession.hostname == "127.0.0.1");
      // REQUIRE(storageClusterSession.port == 5498);
      // NOTE do we want to create the limit of volumes straight away???
      THEN("Four 1GB volumes are created under a single virtual volume") {
	REQUIRE( storageCluster->virtualVolume->id == VolumeId{"a32b8963a2084ba7"} );
	REQUIRE( storageCluster->virtualVolume->storageVolumeManager.volumes.size() == 4 );
	REQUIRE( fs::is_empty(fs::path{"VOLUMES"}) == false );
	// TODO test Marshaller elsewhere

	for (auto kv : storageCluster->virtualVolume->storageVolumeManager.volumes) {
	  // Size is 1GB
	  REQUIRE( kv.second.size == Size{1_gB} );

	  // Allocated size thus far is 0
	  REQUIRE( kv.second->storage->getAllocatedSize() == Size{0_kB} );

	  // Storage is Filesystem storage driver
	  auto storage = std::get<StorageVolume<FileStorage::FilesystemStorage>>(kv.second.storage);
	  REQUIRE( typeof(storage) == typeof(StorageVolume<FileStorage::FilesystemStorage>) );
	  // TODO in volume test, check that limit in JSON config is respected
	}
      }
    }

    // WHEN("the master is spawned with an invalid storage cluster configuration") {
    WHEN("the master is spawned with a valid configuration") {
      master->configure(masterSession.loadConfig(fs::path{"master.json"}));
      masterSession.spawn();

      THEN("the Master configuration matches the file, it creates an empty REGISTRY file") {
	REQUIRE(master->id == UUID4{"9498038e-3e97-45c3-8b92-19073fada165")};
	// TODO networking tests
	// REQUIRE(masterSession.hostname == "127.0.0.1");
	// REQUIRE(masterSession.port == 5498);

	// TODO test for presence of client_nodes and storage_nodes

	REQUIRE( fs::is_empty(fs::path{"REGISTRY"}) == true );
	// REQUIRE();

      AND_WHEN("FileBuckets are created by the FileBucketRegistry") {

	std::vector<std::unique_ptr<file::FileBucket>> fileBuckets;

	auto const& fbRegistry = master->registry;
	// Factory function for adding FileBuckets
	auto addBucket = [&fbRegistry]
	    (auto t, auto t2) {
	  return fbRegistry->create(t, t2);
	};

	for (auto spec : fbArgs) {
	  fileBuckets.emplace_back(std::apply(addBucket, spec));
	}

	THEN("each FileBucket has an associated FileBucketRegistryItem, a virtual volume assigned, mapping to volume ids exists, and its REGISTRY file is populated with the each FileBucketRegistryItem contents") {
	  REQUIRE( fbRegistry->registry.size() == fileBuckets.size() );

	  // Get all volume ids that are in virtual volume
	  std::vector<VolumeId> allVolumeIds;
	  for (auto kv : storageCluster->virtualVolume->storageVolumeManager.volumes) {
	    allVolumeIds.push_back(kv.first);
	  }

	  file::FileBucketRegistryItemConverter converter;

	  for (unsigned int i = 0; i < static_cast<unsigned int>(fileBuckets.size()); i++) {
	    // Acquire the registry item
	    auto registryItem = fbRegistry->registry[i];
	    // Virtual volume is assigned
	    REQUIRE(registryItem->fileBucket.virtualVolumeId == VolumeId{"a32b8963a2084ba7"});
	    // Test that a volume is assigned to this bucket
	    auto volumeIds = storageVolume.virtualVolume.getFileBucketVolumeIds(registryItem->fileBucket.id);
	    // NOTE/TODO: do we want to assign this straight away? if bucket is known to be very large, it will have to "spill over" into different volumes. Test that in volume test
	    REQUIRE(volumeIds.size() == 1);
	    auto fbVolumeId = volumeIds[0];

	    // One of the volumes has been assigned to the bucket
	    // TODO do we want to always evenly distribute the buckets to the volumes?
	    REQUIRE( std::any_of(allVolumeIds.cbegin(), allVolumeIds.cend(), [&fbVolumeId](auto vId){ return fbVolumeId == vId; }) );

	    auto const clonedItem = converter.convertInput(registryItem->contents);
	    REQUIRE( registryItem->contents == clonedItem->contents );
	  }

	  REQUIRE( fs::is_empty(fs::path{"REGISTRY"}) == false );

	  std::string line;
	  auto registryFile = fbRegistry->getRegistry<std::ifstream>();

	  unsigned int counter = 0;
	  auto const len = fbRegistry->registry.size();
	  while (getline(registryFile, line)) {
	    REQUIRE( line == fbRegistry->registry[counter++]->contents );
	  }

	  REQUIRE( counter == fbArgs.size() );
	}
      }
      }

      masterLock.unlock();
      storageClusterLock.unlock();
    }
  }
};

SCENARIO("A CDN with Persisting FileBucket storage is restarted") {

  GIVEN("a persisted MasterNode with persisted buckets and StorageClusterNode with persisted volumes") {
    MasterSession masterSession;
    auto [masterLock, master] = masterSession.getMasterNode();
    master->existing = true;

    StorageClusterSession storageClusterSession;
    auto [storageClusterLock, storageCluster] = storageClusterSession.getStorageClusterNode();
    storageCluster->existing = true;

    WHEN("the storage cluster is spawned") {
      storageClusterSession.spawn();
      THEN("The fileBucket volume DB is initialized within a Virtual Volume and the Storage Cluster node loads the volumes into memory") {
	REQUIRE( storageCluster.virtualVolume->id == VolumeId{"a32b8963a2084ba7"} );
	REQUIRE( storageCluster.virtualVolume.storageVolumeManager.volumes.size() == 4 );

	for (auto kv : storageCluster.virtualVolume->storageVolumeManager.volumes) {
	  // Size is 1GB
	  REQUIRE( kv.second->size == Size{1_gB} );

	  // Allocated size thus far is 0
	  REQUIRE( kv.second->storage->getAllocatedSize() == Size{0_kB} );

	  // Storage is Filesystem storage driver
	  auto storage = std::get<StorageVolume<FileStorage::FilesystemStorage>>(kv.second->storage);
	  REQUIRE( typeof(storage) == typeof(StorageVolume<FileStorage::FilesystemStorage>) );
	}
      }
    }

    WHEN("the master is spawned") {
      masterSession.spawn();
      THEN("The registry is initialized and it loads its FileBuckets into memory") {
	REQUIRE(master->id == UUID4{"9498038e-3e97-45c3-8b92-19073fada165")};

	auto &fbRegistry = master->registry;
	auto findBucket = [&fbRegistry]
	  (auto id) {
	  return fbRegistry->getBucket(id);
	};

	REQUIRE( fbRegistry->registry.size() == fbArgs.size() );

	// using fbTestProps = std::tuple<FileBucketId&, Size, Size&, std::vector<std::string>&>;

	// Get all volume ids that are in virtual volume
	std::vector<VolumeId> allVolumeIds;
	for (auto kv : storageCluster->virtualVolume->storageVolumeManager.volumes) {
	  allVolumeIds.push_back(kv.first);
	}

	std::vector<std::unique_ptr<Size>> fbSizes;
	std::vector<std::vector<std::string>> fbTypes;
	// TODO test ids from previous scenario
	// std::vector<FileBucketId> fbIds;

	for (unsigned int i = 0; i < static_cast<unsigned int>(fbArgs.size()); i++) {
	  auto const fbArg = fbArgs[i];
	  auto bucket = std::apply(findBucket, fbArg);
	  auto const& registryItem = fbRegistry->registry[i];
	  std::cout << "registryItem: " << registryItem->contents << "\n";

	  // Ownership of the FileBucket was transferred to this scope
	  REQUIRE( registryItem->fileBucket.has_value() == false );

	  // Test the fields of the FileBucket
	  fbSizes.emplace_back(std::make_unique<Size>(static_cast<Size>(std::get<0>(fbArgs[i]))));
	  fbTypes.emplace_back(static_cast<std::vector<std::string>>(std::get<1>(fbArgs[i])));
	  // FileBucketId fbId{std::string{i+1}};
	  // fbIds.emplace_back(fbId);

	  // REQUIRE( bucket->id == fbIds[i] );
	  REQUIRE( bucket->virtualVolumeId == VolumeId{"a32b8963a2084ba7"} );
	  REQUIRE( bucket->size == *fbSizes[i].get() );

	  REQUIRE( fbRegistry->registry.size() == fbArgs.size() );

	  // Test FileBucket allocated size
	  REQUIRE( bucket->getAllocatedSize() == Size{0_kB} );

	  // Test that a volume was assigned to this bucket
	  auto volumeIds = storageVolume.virtualVolume.getFileBucketVolumeIds(registryItem->fileBucket.id);
	  REQUIRE(volumeIds.size() == 1);
	  auto fbVolumeId = volumeIds[0];

	  // One of the volumes has been assigned to the bucket
	  // TODO make sure volume id is the same as before
	  REQUIRE( std::any_of(allVolumeIds.cbegin(), allVolumeIds.cend(), [&fbVolumeId](auto vId){ return fbVolumeId == vId; }) );
	}
      }
    }

    // Tear down
    fs::remove("VOLUMES");
    fs::remove("REGISTRY");

    masterLock.unlock();
    storageClusterLock.unlock();
  }
};
