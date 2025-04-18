#pragma once
#include <zlib.h>
#include <QByteArray>
#include <QFile>

class GZip {
   public:
    static bool unzip(const QByteArray& compressedBytes, QByteArray& uncompressedBytes);
    static bool zip(const QByteArray& uncompressedBytes, QByteArray& compressedBytes);
};

class GZipStream {
   public:
    explicit GZipStream(const QString& filePath);
    explicit GZipStream(QFile* file);

    // Decompress the next block and return the decompressed data
    bool unzipBlockByBlock(QByteArray& uncompressedBytes);

   private:
    bool initStream();

    bool processBlock(const QByteArray& compressedBlock, QByteArray& uncompressedBytes);

   private:
    QFile* m_file;
    z_stream m_strm;
};
