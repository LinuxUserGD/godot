#ifndef FILE_ACCESS_WII_H
#define FILE_ACCESS_WII_H

#include "core/os/file_access.h"

#include <stdio.h>

class FileAccessWii : public FileAccess {
protected:
    FILE *fp;
    String path_src;
    String path;
    mutable Error last_error;
    int flags;

    void check_errors() const;
public:
    virtual Error _open(const String &p_path, int p_mode_flags); // Open file
    virtual void close(); // Close file
    virtual bool is_open() const; // Is file open?

    virtual String get_path() const;
    virtual String get_path_absolute() const;

    virtual void seek(size_t p_position); // Seek to position in file
    virtual void seek_end(int64_t p_position = 0); // Seek to position from end of file
    virtual size_t get_position() const; // Get current file position
    virtual size_t get_len() const; // Get size of file

    virtual bool eof_reached() const; // Reached end of file?
    
    virtual uint8_t get_8() const; // Read a byte
    virtual int get_buffer(uint8_t *p_dst, int p_length) const; // Read a buffer of bytes

    virtual Error get_error() const; // Get last error

    virtual void flush(); // Flush buffer to file
    virtual void store_8(uint8_t p_dest); // Store byte
    virtual void store_buffer(const uint8_t *p_src, int p_length); // Store a buffer of bytes

    virtual bool file_exists(const String &p_path); // Does file at p_path exist?

    virtual uint64_t _get_modified_time(const String &p_file);
    virtual uint32_t _get_unix_permissions(const String &p_file);
    virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions);

    FileAccessWii();
    ~FileAccessWii();
};

#endif