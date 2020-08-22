#ifndef DIR_ACCESS_WII_H
#define DIR_ACCESS_WII_H

#include "core/os/dir_access.h"

#include <dirent.h>

class DirAccessWii : public DirAccess
{
    DIR *dir_stream;

    String current_dir;
	bool _cisdir;
	bool _cishidden;

protected:
    virtual String fix_unicode_name(const char *p_name) const { return String::utf8(p_name); }

public:
    virtual Error list_dir_begin();
    virtual String get_next();
    virtual bool current_is_dir() const;
    virtual bool current_is_hidden() const;

    virtual void list_dir_end();

    virtual int get_drive_count();
    virtual String get_drive(int p_drive);

    virtual Error change_dir(String p_dir);
    virtual String get_current_dir();
    virtual Error make_dir(String p_dir);

    virtual bool file_exists(String p_file);
    virtual bool dir_exists(String p_dir);

    virtual uint64_t get_modified_time(String p_file);

    virtual size_t get_space_left();

    virtual Error rename(String p_path, String p_new_path);
    virtual Error remove(String p_name);

    virtual String get_filesystem_type() const;

    DirAccessWii();
    virtual ~DirAccessWii();
};

#endif