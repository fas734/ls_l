#include <limits.h> // PATH_MAX
#include <stdlib.h> // exit()
#include <errno.h>  // perror()
#include <stdio.h>
#include <string.h>
#include <unistd.h> // getcwd()
#include <dirent.h> // DIR
#include <sys/stat.h> // struct stat
#include <pwd.h>    // getpwuid()
#include <grp.h>    // getgrgid()
#include <time.h>   // ctime()


typedef struct file_info_struct
{
    unsigned long inode;
    char mode[11];
    unsigned long nlink;
    char owner_name[33];
    char group_name[33];
    unsigned long long size;
    unsigned int year;
    char month[4];
    unsigned int mday;
    unsigned int hours;
    unsigned int mins;
    char filename[255];
} file_info_struct;


void set_filetype_permissions(file_info_struct *file_info_st, unsigned int mode);
void set_datetime(file_info_struct *file_info_st, struct tm *datetime);
int compare_file_info_struct_filenames(const void *struct1, const void *struct2);


int main(int argc, const char *argv[])
{
    int file_count = 0;
    char cur_dir_name[PATH_MAX];
    DIR *cur_dir_ptr = NULL;
    struct dirent *dir_entry = NULL;
    struct stat file_info;
    file_info_struct *files_info = NULL;
    // for formatting purposes
    unsigned int max_nlink = 1,
                 max_length_nlink = 1,
                 max_length_owner = 1,
                 max_length_group = 1,
                 max_size         = 1,
                 max_length_size  = 1;
    char *format_string_array[] = {
                "%s %", /* drwxrwxrwx */ \
                "lu %-", /* nlink */ \
                "s %-",  /* owner */ \
                "s %",  /* group */ \
                "llu %s %2u %02u:%02u %s\n"}; /* size, remainder of line */
    unsigned int format_string_array_size = (sizeof(format_string_array) \
                                        / sizeof(format_string_array[0]));
    char *format_string = NULL; // for resulting format string
    unsigned int format_string_size = 0;


    // find current directory name and open that directory
    if(getcwd(cur_dir_name, sizeof(cur_dir_name)) == NULL)
    {
        perror("getcwd() error");
        exit(1);
    }
    cur_dir_ptr = opendir(cur_dir_name);
    if(!cur_dir_ptr)
    {
        perror("opendir() error");
        exit(1);
    }

    // count files in current directory and reserve memory for files_info
    while((dir_entry = readdir(cur_dir_ptr)) != NULL)
    {
        file_count++;
    }
    files_info = malloc(file_count * sizeof(file_info_struct));
    if(files_info == NULL)
    {
        perror("malloc(file_info_struct) error");
        exit(1);
    }

    // reset pointer, infill files_info and count max_lengths
    rewinddir(cur_dir_ptr);
    for(int i = 0; i < file_count; i++)
    {
        if((dir_entry = readdir(cur_dir_ptr)) == NULL)
        {
            break;
        }
        if(stat(dir_entry->d_name, &file_info) == -1) // statx gives more info
        {
            perror("stat()");
            exit(EXIT_FAILURE);
        }

        (files_info + i) -> inode = (long)file_info.st_ino;

        set_filetype_permissions((files_info + i), file_info.st_mode);

        (files_info + i) -> nlink = (long)file_info.st_nlink;
        if(max_nlink < (files_info + i) -> nlink)
            max_nlink = (files_info + i) -> nlink;

        strcpy((files_info + i) -> owner_name, \
               getpwuid(file_info.st_uid) -> pw_name); // get username by uid
        (files_info + i) -> owner_name[32] = '\0';
        if(max_length_owner < strlen((files_info + i) -> owner_name))
            max_length_owner = strlen((files_info + i) -> owner_name);

        strcpy((files_info + i) -> group_name, \
               getgrgid(file_info.st_gid) -> gr_name); // get groupname by gid
        (files_info + i) -> group_name[32] = '\0';
        if(max_length_group < strlen((files_info + i) -> group_name))
            max_length_group = strlen((files_info + i) -> group_name);

        (files_info + i) -> size = (long long)file_info.st_size;
        if(max_size < (files_info + i) -> size)
            max_size = (files_info + i) -> size;

        #if defined(__APPLE__)
        time_t file_info_time = file_info.st_mtimespec.tv_sec;
        #else
        time_t file_info_time = file_info.st_mtim.tv_sec;
        #endif
        set_datetime((files_info + i), localtime(&file_info_time));
        // date
        // time
        strcpy((files_info + i)->filename, dir_entry->d_name);
    }
    closedir(cur_dir_ptr);

    // sort files_info by filenames in ASCII order
    qsort(files_info, file_count, sizeof(file_info_struct), \
            compare_file_info_struct_filenames);

    // preparing format output
    while(max_nlink >= 10)
    {
        max_nlink /= 10;
        max_length_nlink++;
    }
    while(max_size >= 10)
    {
        max_size /= 10;
        max_length_size++;
    }
    for(int i = 0; i < format_string_array_size; i++)
    {
        format_string_size += strlen(format_string_array[i]);
    }
    format_string_size += max_length_nlink \
                        + max_length_owner \
                        + max_length_group \
                        + max_length_size;
    // printf("format_string_size = %u\n", format_string_size); // that size a little bit more than it has to
    format_string = malloc((format_string_size + 1) * sizeof(char)); // '+1' for '\0'
    if(format_string == NULL)
    {
        perror("malloc(format_string[]) error");
        exit(1);
    }
    sprintf(format_string, "%s", format_string_array[0]);
    sprintf(format_string + strlen(format_string), "%u", max_length_nlink);
    sprintf(format_string + strlen(format_string), "%s", format_string_array[1]);
    sprintf(format_string + strlen(format_string), "%u", max_length_owner);
    sprintf(format_string + strlen(format_string), "%s", format_string_array[2]);
    sprintf(format_string + strlen(format_string), "%u", max_length_group);
    sprintf(format_string + strlen(format_string), "%s", format_string_array[3]);
    sprintf(format_string + strlen(format_string), "%u", max_length_size);
    sprintf(format_string + strlen(format_string), "%s", format_string_array[4]);

    // show result
    for(int i=0; i<file_count; i++)
    {
        printf(format_string, \
                (files_info + i) -> mode, \
                (files_info + i) -> nlink, \
                (files_info + i) -> owner_name, \
                (files_info + i) -> group_name, \
                (files_info + i) -> size, \
                (files_info + i) -> month, \
                (files_info + i) -> mday, \
                (files_info + i) -> hours, \
                (files_info + i) -> mins, \
                (files_info + i) -> filename);
    }
    free(files_info);
    free(format_string);

    return 0;
}


void set_filetype_permissions(file_info_struct *file_info_st, unsigned int mode)
{
    // type of file
    switch (mode & S_IFMT)
    {
        case S_IFSOCK: file_info_st -> mode[0] = 's';  break;
        case S_IFLNK:  file_info_st -> mode[0] = 'l';  break;
        case S_IFREG:  file_info_st -> mode[0] = '-';  break;
        case S_IFBLK:  file_info_st -> mode[0] = 'b';  break;
        case S_IFDIR:  file_info_st -> mode[0] = 'd';  break;
        case S_IFCHR:  file_info_st -> mode[0] = 'c';  break;
        case S_IFIFO:  file_info_st -> mode[0] = 'p';  break;
        default:       file_info_st -> mode[0] = '?';  break;
    }
    // user permissions
    file_info_st -> mode[1] = (mode & S_IRUSR) ? 'r' : '-';
    file_info_st -> mode[2] = (mode & S_IWUSR) ? 'w' : '-';
    file_info_st -> mode[3] = (mode & S_IXUSR) ? 'x' : '-';
    // group permissions
    file_info_st -> mode[4] = (mode & S_IRGRP) ? 'r' : '-';
    file_info_st -> mode[5] = (mode & S_IWGRP) ? 'w' : '-';
    file_info_st -> mode[6] = (mode & S_IXGRP) ? 'x' : '-';
    // others permissions
    file_info_st -> mode[7] = (mode & S_IROTH) ? 'r' : '-';
    file_info_st -> mode[8] = (mode & S_IWOTH) ? 'w' : '-';
    file_info_st -> mode[9] = (mode & S_IXOTH) ? 'x' : '-';

    if(mode & S_ISUID)  // if set-user-ID bit
        file_info_st -> mode[3] = (mode & S_IXUSR) ? 's' : 'S';

    if(mode & S_ISGID)  // if set-group-ID bit
        file_info_st -> mode[6] = (mode & S_IXGRP) ? 's' : 'S';

    if(mode & S_ISVTX)  // if sticky bit
        file_info_st -> mode[9] = (mode & S_IXOTH) ? 't' : 'T';

    file_info_st -> mode[10] = '\0';
}


void set_datetime(file_info_struct *file_info_st, struct tm *datetime)
{
    file_info_st -> year  = (datetime -> tm_year) + 1900;
    switch (datetime -> tm_mon)
    {
        case  0: strcpy(file_info_st -> month, "Jan\0");  break;
        case  1: strcpy(file_info_st -> month, "Feb\0");  break;
        case  2: strcpy(file_info_st -> month, "Mar\0");  break;
        case  3: strcpy(file_info_st -> month, "Apr\0");  break;
        case  4: strcpy(file_info_st -> month, "May\0");  break;
        case  5: strcpy(file_info_st -> month, "Jun\0");  break;
        case  6: strcpy(file_info_st -> month, "Jul\0");  break;
        case  7: strcpy(file_info_st -> month, "Aug\0");  break;
        case  8: strcpy(file_info_st -> month, "Sep\0");  break;
        case  9: strcpy(file_info_st -> month, "Oct\0");  break;
        case 10: strcpy(file_info_st -> month, "Nov\0");  break;
        case 11: strcpy(file_info_st -> month, "Dec\0");  break;
    }
    file_info_st -> mday  = datetime -> tm_mday;
    file_info_st -> hours = datetime -> tm_hour;
    file_info_st -> mins  = datetime -> tm_min;
}


int compare_file_info_struct_filenames(const void *struct1, const void *struct2)
{
    char *filename1 = (*(file_info_struct*)struct1).filename;
    char *filename2 = (*(file_info_struct*)struct2).filename;
    return strcmp(filename1, filename2);
}
