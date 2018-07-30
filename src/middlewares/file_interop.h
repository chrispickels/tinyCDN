#include "../master.hpp"
#ifdef __cplusplus
#include <string.h>
#include <optional>
#include <memory>
#include "file.hpp"
#include "utility.hpp"
#include "FileStorage/storedfile.hpp"
#endif

#ifdef __cplusplus
namespace TinyCDN {
extern "C" {
  struct FileUploadingSession {
    std::unique_ptr<Middleware::File::FileBucket> bucket;
    std::unique_ptr<Middleware::FileStorage::StoredFile> uploadingFile;
    std::unique_ptr<Middleware::File::FileUploadingService> uploadService;
  };
  struct FileHostingSession {
    std::unique_ptr<Middleware::File::FileBucket> bucket;
    std::unique_ptr<Middleware::FileStorage::StoredFile> hostingFile;
    std::unique_ptr<Middleware::File::FileHostingService> hostingService;

    std::unique_ptr<ChunkedCursor> cursor;
  };
#else
typedef struct FileUploadingSession FileUploadingSession;
typedef struct FileHostingSession FileHostingSession;
typedef struct ChunkedCursor ChunkedCursor;
#endif

  struct FileUploadInfo {
    char* temporaryLocation;
    char* contentType;
    char* fileType;
    int wantsOwned;
    char* tags;
#ifdef __cplusplus
    inline FileUploadInfo(char* t,
                          char* ct,
                          char* ft,
                          char* tgs,
                          int owned)
      : wantsOwned(owned) {
      temporaryLocation = new char[strlen(t)+1];
      strcpy(temporaryLocation, t);

      contentType = new char[strlen(ct)+1];
      strcpy(contentType, ct);

      fileType = new char[strlen(ft)+1];
      strcpy(fileType, ft);

      tags = new char[strlen(tgs)+1];
      // TODO copy tags?
    }
    #endif
  };

  struct FileUploadInfo* FileUploadInfo_new (char* temporaryLocation,
                                             char* contentType,
                                             char* fileType,
                                             char* tags,
                                             int wantsOwned);

  struct FileUploadingSession* FileUploadingSession_new ();
  void FileUploadingSession_delete (struct FileUploadingSession* session);
  int FileUploadingSession_getBucket (struct FileUploadingSession* session, struct FileUploadInfo* info);
  void FileUploadingSession_finishFileUpload (struct FileUploadingSession* session, char* cffiResult[], struct FileUploadInfo* info);

  struct FileHostingSession* FileHostingSession_new ();
  void FileHostingSession_delete (struct FileHostingSession* session);
  int FileHostingSession_getBucket (struct FileHostingSession* session, int id);
  void FileHostingSession_getHandle (struct FileHostingSession* session);
  char* FileHostingSession_yieldChunk (struct FileHostingSession* session, char* cffiResult[]);

  // C++ functions
  struct FileUploadInfo* cc_FileUploadInfo_new (char* temporaryLocation,
                                                char* contentType,
                                                char* fileType,
                                                char* tags,
                                                int wantsOwned);

  struct FileUploadingSession* cc_FileUploadingSession_new ();
  void cc_FileUploadingSession_delete (struct FileUploadingSession* session);
  int cc_FileUploadingSession_getBucket (struct FileUploadingSession* session, struct FileUploadInfo* info);
  void cc_FileUploadingSession_finishFileUpload (struct FileUploadingSession* session, char* cffiResult[], struct FileUploadInfo* info);

  struct FileHostingSession* cc_FileHostingSession_new ();
  void cc_FileHostingSession_delete (struct FileHostingSession* session);
  int cc_FileHostingSession_getBucket (struct FileHostingSession* session, int id);
  void cc_FileHostingSession_chunkFile (struct FileHostingSession* session);
  char* cc_FileHostingSession_yieldChunk (struct FileHostingSession* session, char* cffiResult[]);

#ifdef __cplusplus
}
}
#endif